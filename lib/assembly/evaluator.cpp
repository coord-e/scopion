/**
* @file evaluator.cpp
*
* (c) copyright 2017 coord.e
*
* This file is part of scopion.
*
* scopion is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* scopion is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with scopion.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "scopion/assembly/evaluator.hpp"
#include "scopion/assembly/translator.hpp"
#include "scopion/assembly/value.hpp"

#include "scopion/ast/expr.hpp"
#include "scopion/ast/util.hpp"
#include "scopion/ast/value.hpp"

#include <llvm/AsmParser/Parser.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_os_ostream.h>

#include <boost/range/adaptor/indexed.hpp>

#include <cassert>
#include <string>
#include <vector>

namespace scopion
{
namespace assembly
{
std::pair<bool, value*> apply_bb(ast::scope const& sc, translator& tr)
{
  auto insts = ast::val(sc);
  value* ll  = nullptr;
  for (auto it = insts.begin(); it != insts.end(); it++) {
    ll = boost::apply_visitor(tr, *it);
  }
  auto cb = tr.getBuilder().GetInsertBlock();
  return std::make_pair(std::none_of(cb->begin(), cb->end(),
                                     [](auto& i) {
                                       return i.getOpcode() == llvm::Instruction::Ret ||
                                              i.getOpcode() == llvm::Instruction::Br;
                                     }),
                        ll);
}

evaluator::evaluator(value* v, std::vector<value*> const& args, translator& tr)
    : v_(v), arguments_(args), translator_(tr), builder_(tr.builder_)
{
}

value* evaluator::operator()(ast::value const& astv)
{
  return boost::apply_visitor(*this, astv);
}

value* evaluator::operator()(ast::operators const& astv)
{
  return boost::apply_visitor(*this, astv);
}

value* evaluator::operator()(ast::function const& fcv)
{
  if (v_->getType()->getLLVM()->getPointerElementType()->getFunctionNumParams() !=
      arguments_.size())
    throw error("The number of arguments doesn't match: required " +
                    std::to_string(
                        v_->getType()->getLLVM()->getPointerElementType()->getFunctionNumParams()) +
                    " but supplied " + std::to_string(arguments_.size()),
                ast::attr(fcv).where, errorType::Translate);

  std::vector<std::string> arg_names;
  std::vector<llvm::Type*> arg_types_from_attr;
  std::transform(
      ast::val(fcv).first.begin(), ast::val(fcv).first.end(), std::back_inserter(arg_names),
      [&arg_types_from_attr, &fcv, this](auto& x) {
        auto it       = ast::attr(x).attributes.find("type");
        auto ito      = ast::attr(x).attributes.find("typeof");
        llvm::Type* t = nullptr;
        if (it != ast::attr(x).attributes.end()) {
          llvm::SMDiagnostic err;
          t = llvm::parseType(it->second, err, *(translator_.module_->getLLVMModule()));
          if (!t) {
            llvm::raw_os_ostream stream(std::cerr);
            err.print("", stream);
            throw error("Failed to parse type name \"" + it->second + "\"", ast::attr(fcv).where,
                        errorType::Translate);
          }
        } else if (ito != ast::attr(x).attributes.end()) {
          auto var = translator_(ast::set_where(ast::variable(ito->second), ast::attr(x).where));
          t        = var->getType()->getLLVM();
          assert(t && "Type got from #typeof was nullptr!");
        }
        arg_types_from_attr.push_back(t);
        return ast::val(x);  // x(ast::identifier)
      });

  llvm::Type* retst = nullptr;

  {
    auto it  = ast::attr(fcv).attributes.find("rettype");
    auto ito = ast::attr(fcv).attributes.find("rettypeof");
    if (it != ast::attr(fcv).attributes.end()) {
      llvm::SMDiagnostic err;
      retst = llvm::parseType(it->second, err, *(translator_.module_->getLLVMModule()));
      if (!retst) {
        llvm::raw_os_ostream stream(std::cerr);
        err.print("", stream);
        throw error("Failed to parse type name \"" + it->second + "\"", ast::attr(fcv).where,
                    errorType::Translate);
      }
    } else if (ito != ast::attr(fcv).attributes.end()) {
      auto var = translator_(ast::set_where(ast::variable(ito->second), ast::attr(fcv).where));
      retst    = var->getType()->getLLVM();
    }
  };

  std::vector<llvm::Type*> arg_types;
  std::vector<llvm::Type*> arg_types_for_func;

  for (auto const v : arguments_ | boost::adaptors::indexed()) {
    auto type = v.value()->getType()->getLLVM();
    auto expt = arg_types_from_attr[static_cast<unsigned long>(v.index())];
    if (expt && expt != type)
      throw error("Type mismatch on argument No." + std::to_string(v.index()) + ": expected \"" +
                      getNameString(expt) + "\" but supplied \"" + getNameString(type) + "\"",
                  ast::attr(fcv).where, errorType::Translate);
    if (!v.value()->getType()->isLazy())
      arg_types_for_func.push_back(type);
    arg_types.push_back(v.value()->getType()->isFundamental() ? type
                                                              : type->getPointerElementType());
  }

  llvm::FunctionType* func_type =
      llvm::FunctionType::get(retst ? retst : builder_.getVoidTy(), arg_types_for_func, false);
  llvm::Function* func =
      llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, v_->getLLVM()->getName(),
                             translator_.module_->getLLVMModule());
  llvm::BasicBlock* entry =
      llvm::BasicBlock::Create(translator_.module_->getContext(), "entry_survey", func);
  auto prevScope = translator_.getScope();
  translator_.setScope(new value(entry, fcv));

  auto pb = builder_.GetInsertBlock();
  auto pp = builder_.GetInsertPoint();

  builder_.SetInsertPoint(entry);

  translator_.getScope()->symbols()["__self"] =
      new value(builder_.CreateAlloca(func->getType(), nullptr, "__self"), fcv);

  for (auto const arg_name : arg_names | boost::adaptors::indexed()) {
    auto ulindex = static_cast<unsigned long>(arg_name.index());
    auto vp      = arguments_[ulindex];
    if (!vp->getType()->isLazy()) {
      translator_.getScope()->symbols()[arg_name.value()] =
          vp->copyWithNewLLVMValue(builder_.CreateAlloca(arg_types[ulindex], nullptr,
                                                         arg_name.value()));  // declare arguments
    } else {
      translator_.getScope()->symbols()[arg_name.value()] = vp;
    }
  }

  ret_table_t* ret_table = nullptr;
  for (auto const& line : ast::val(fcv).second) {
    auto e = ast::set_survey(line, true);
    auto v = boost::apply_visitor(translator_, e);
    if (!ret_table)
      ret_table = v->getRetTable();
  }

  llvm::Type* ret_type = nullptr;
  for (auto const& bb : *func) {
    for (auto itr = bb.getInstList().begin(); itr != bb.getInstList().end(); ++itr) {
      if ((*itr).getOpcode() == llvm::Instruction::Ret) {
        if ((*itr).getOperand(0)->getType() != ret_type) {
          if (ret_type == nullptr) {
            ret_type = (*itr).getOperand(0)->getType();
          } else {
            throw error("All return values must have the same type", ast::attr(fcv).where,
                        errorType::Translate);
          }
        }
      }
    }
  }

  if (!ret_type) {
    builder_.CreateRetVoid();
    ret_type = builder_.getVoidTy();
  }

  if (retst && retst != ret_type) {
    throw error("Return type doesn't match: expected \"" + getNameString(retst) +
                    "\" but supplied \"" + getNameString(ret_type) + "\"",
                ast::attr(fcv).where, errorType::Translate);
  }

  func->eraseFromParent();  // remove old one

  llvm::Function* newfunc;
  if (ast::attr(fcv).survey) {  // is it really ok?
    newfunc = llvm::Function::Create(llvm::FunctionType::get(ret_type, arg_types_for_func, false),
                                     llvm::Function::ExternalLinkage);
  } else {  // Create the real content of function if it
            // isn't in survey

    newfunc = llvm::Function::Create(llvm::FunctionType::get(ret_type, arg_types_for_func, false),
                                     llvm::Function::ExternalLinkage, v_->getLLVM()->getName(),
                                     translator_.module_->getLLVMModule());

    llvm::BasicBlock* newentry =
        llvm::BasicBlock::Create(translator_.module_->getContext(), "entry", newfunc);

    translator_.setScope(new value(newentry, fcv));
    builder_.SetInsertPoint(newentry);

    auto selfptr = builder_.CreateAlloca(newfunc->getType(), nullptr, "__self");
    translator_.getScope()->symbols()["__self"] = new value(selfptr, fcv);
    builder_.CreateStore(newfunc, selfptr);

    auto ait = newfunc->arg_begin();
    for (auto const arg_name : arg_names | boost::adaptors::indexed()) {
      auto ulindex = static_cast<unsigned long>(arg_name.index());
      auto argv    = arguments_[ulindex];
      if (!argv->getType()->isLazy()) {
        auto aptr = builder_.CreateAlloca(arg_types[ulindex], nullptr,
                                          arg_name.value());  // declare arguments
        translator_.getScope()->symbols()[arg_name.value()] = argv->copyWithNewLLVMValue(aptr);
        builder_.CreateStore(argv->getType()->isFundamental() ? static_cast<llvm::Value*>(&(*ait))
                                                              : builder_.CreateLoad(&(*ait)),
                             aptr);
        ait++;
      } else {
        // std::cout << arg_name.value() << " is lazy" << std::endl;
        translator_.getScope()->symbols()[arg_name.value()] = argv;
      }
    }

    for (auto const& line : ast::val(fcv).second) {
      boost::apply_visitor(translator_, line);
    }

    if (ret_type->isVoidTy()) {
      builder_.CreateRetVoid();
    }
  }

  translator_.setScope(prevScope);
  builder_.SetInsertPoint(pb, pp);

  // value.getValue()->eraseFromParent();

  auto destv = new value(newfunc, fcv);
  destv->setRetTable(ret_table);
  return destv;
}

value* evaluator::operator()(ast::scope const& sc)
{
  auto pb = builder_.GetInsertBlock();
  auto pp = builder_.GetInsertPoint();

  auto nb = llvm::BasicBlock::Create(translator_.module_->getContext(), "",
                                     builder_.GetInsertBlock()->getParent());

  auto theblock = llvm::BasicBlock::Create(translator_.module_->getContext(), "",
                                           builder_.GetInsertBlock()->getParent());
  builder_.SetInsertPoint(theblock);
  auto prevScope = translator_.getScope();

  translator_.setScope(v_);
  auto r      = apply_bb(sc, translator_);
  bool non_rb = r.first;
  if (non_rb)
    builder_.CreateBr(nb);
  else
    nb->eraseFromParent();

  translator_.setScope(prevScope);

  builder_.SetInsertPoint(pb, pp);

  builder_.CreateBr(theblock);

  if (non_rb)
    builder_.SetInsertPoint(nb);

  return r.second;  // lastline
}

value* evaluate(value* v, std::vector<value*> const& args, translator& tr)
{
  auto evor = evaluator{v, args, tr};
  return v->getType()->isLazy() ? boost::apply_visitor(evor, v->getAst()) : v->copy();
}
}
}
