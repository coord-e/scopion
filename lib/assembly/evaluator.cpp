
#include "scopion/assembly/evaluator.hpp"
#include "scopion/assembly/translator.hpp"
#include "scopion/assembly/value.hpp"

#include "scopion/ast/expr.hpp"
#include "scopion/ast/util.hpp"
#include "scopion/ast/value.hpp"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <boost/range/adaptor/indexed.hpp>

#include <cassert>
#include <vector>

namespace scopion
{
namespace assembly
{
bool apply_bb(ast::scope const& sc, translator& tr)
{
  auto insts = ast::val(sc);
  for (auto it = insts.begin(); it != insts.end(); it++) {
    boost::apply_visitor(tr, *it);
  }
  auto cb = tr.builder_.GetInsertBlock();
  return std::none_of(cb->begin(), cb->end(), [](auto& i) {
    return i.getOpcode() == llvm::Instruction::Ret || i.getOpcode() == llvm::Instruction::Br;
  });
}

evaluator::evaluator(value* v, std::vector<value*> const& args, translator& tr)
    : v_(v), arguments_(args), translator_(tr), builder_(tr.builder_), module_(tr.module_)
{
}

llvm::Value* evaluator::operator()(ast::value const& astv)
{
  return boost::apply_visitor(*this, astv);
}

llvm::Value* evaluator::operator()(ast::operators const& astv)
{
  return boost::apply_visitor(*this, astv);
}

llvm::Value* evaluator::operator()(ast::function const& fcv)
{
  if (v_->getLLVM()->getType()->getPointerElementType()->getFunctionNumParams() !=
      arguments_.size())
    throw error("The number of arguments doesn't match: required " +
                    std::to_string(
                        v_->getLLVM()->getType()->getPointerElementType()->getFunctionNumParams()) +
                    " but supplied " + std::to_string(arguments_.size()),
                ast::attr(fcv).where, translator_.code_range_);

  std::vector<std::string> arg_names;
  std::transform(ast::val(fcv).first.begin(), ast::val(fcv).first.end(),
                 std::back_inserter(arg_names), [](auto& x) {
                   return ast::val(x);  // x(ast::identifier)
                 });

  std::vector<llvm::Type*> arg_types;
  std::vector<llvm::Type*> arg_types_for_func;

  for (auto const& arg_value : arguments_) {
    auto type = arg_value->getLLVM()->getType();
    if (!arg_value->isLazy())
      arg_types_for_func.push_back(type);
    arg_types.push_back(type);
  }

  llvm::FunctionType* func_type =
      llvm::FunctionType::get(builder_.getVoidTy(), arg_types_for_func, false);
  llvm::Function* func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage,
                                                v_->getLLVM()->getName(), module_.get());
  llvm::BasicBlock* entry = llvm::BasicBlock::Create(module_->getContext(), "entry_survey", func);
  auto prevScope          = translator_.getScope();
  translator_.setScope(new value(entry, fcv));

  auto pb = builder_.GetInsertBlock();
  auto pp = builder_.GetInsertPoint();

  builder_.SetInsertPoint(entry);

  translator_.getScope()->symbols()["__self"] =
      new value(translator_.createGCMalloc(func->getType(), nullptr, "__self"), fcv);

  for (auto const& arg_name : arg_names | boost::adaptors::indexed()) {
    auto vp = arguments_[arg_name.index()];
    if (!vp->isLazy()) {
      translator_.getScope()->symbols()[arg_name.value()] = vp->copyWithNewLLVMValue(
          translator_.createGCMalloc(arg_types[arg_name.index()], nullptr,
                                     arg_name.value()));  // declare arguments
    } else {
      translator_.getScope()->symbols()[arg_name.value()] = vp;
    }
  }

  for (auto const& line : ast::val(fcv).second) {
    auto e = ast::set_survey(line, true);
    boost::apply_visitor(translator_, e);
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
                        translator_.code_range_);
          }
        }
      }
    }
  }

  if (!ret_type) {
    builder_.CreateRetVoid();
    ret_type = builder_.getVoidTy();
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
                                     module_.get());

    llvm::BasicBlock* newentry = llvm::BasicBlock::Create(module_->getContext(), "entry", newfunc);

    translator_.setScope(new value(newentry, fcv));
    builder_.SetInsertPoint(newentry);

    auto selfptr = translator_.createGCMalloc(newfunc->getType(), nullptr, "__self");
    translator_.getScope()->symbols()["__self"] = new value(selfptr, fcv);
    builder_.CreateStore(newfunc, selfptr);

    auto it = newfunc->getArgumentList().begin();
    for (auto const& arg_name : arg_names | boost::adaptors::indexed()) {
      auto argv = arguments_[arg_name.index()];
      if (!argv->isLazy()) {
        auto aptr = translator_.createGCMalloc(arg_types[arg_name.index()], nullptr,
                                               arg_name.value());  // declare arguments
        translator_.getScope()->symbols()[arg_name.value()] = argv->copyWithNewLLVMValue(aptr);
        builder_.CreateStore(&(*it), aptr);
        it++;
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

  return newfunc;
}

llvm::Value* evaluator::operator()(ast::scope const& sc)
{
  auto pb = builder_.GetInsertBlock();
  auto pp = builder_.GetInsertPoint();

  auto nb =
      llvm::BasicBlock::Create(module_->getContext(), "", builder_.GetInsertBlock()->getParent());

  auto theblock =
      llvm::BasicBlock::Create(module_->getContext(), "", builder_.GetInsertBlock()->getParent());
  builder_.SetInsertPoint(theblock);
  auto prevScope = translator_.getScope();

  translator_.setScope(v_);
  bool non_rb = apply_bb(sc, translator_);
  if (non_rb)
    builder_.CreateBr(nb);
  else
    nb->eraseFromParent();

  translator_.setScope(prevScope);

  builder_.SetInsertPoint(pb, pp);

  builder_.CreateBr(theblock);

  if (non_rb)
    builder_.SetInsertPoint(nb);

  return nullptr;  // void
}

value* evaluate(value* v, std::vector<value*> const& args, translator& tr)
{
  auto evor           = evaluator{v, args, tr};
  llvm::Value* evaled = v->isLazy() ? boost::apply_visitor(evor, v->getAst()) : v->getLLVM();
  auto destv          = v->copyWithNewLLVMValue(evaled);
  destv->isLazy(false);
  return destv;
}
};
};
