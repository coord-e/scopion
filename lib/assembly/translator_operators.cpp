/**
* @file translator_operators.cpp
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

#include "scopion/error.hpp"

#include <boost/range/adaptor/indexed.hpp>

#include <algorithm>
#include <iostream>
#include <limits>

namespace scopion
{
namespace assembly
{
llvm::Value* translator::createGCMalloc(llvm::Type* Ty,
                                        llvm::Value* ArraySize,
                                        const llvm::Twine& Name)
{
  if (std::find(module_->link_libraries_.begin(), module_->link_libraries_.end(), "gc") ==
      module_->link_libraries_.end()) {
    module_->getLLVMModule()->getOrInsertFunction(
        "GC_init", llvm::FunctionType::get(builder_.getVoidTy(), false));
    module_->getLLVMModule()->getOrInsertFunction(
        "GC_malloc",
        llvm::FunctionType::get(builder_.getInt8Ty()->getPointerTo(),
                                llvm::ArrayRef<llvm::Type*>({builder_.getInt64Ty()}), false));
    insertGCInitInMain();
    module_->link_libraries_.push_back("gc");
  }
  assert(!ArraySize &&
         "Parameter ArraySize is for compatibility with IRBuilder<>::CreateAlloca. Don't pass any "
         "value.");
  std::vector<llvm::Value*> idxList = {builder_.getInt32(1)};
  auto sizelp                       = builder_.CreatePtrToInt(
      builder_.CreateGEP(
          Ty, llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(Ty->getPointerTo())),
          idxList),
      builder_.getInt64Ty());  // ptrtoint %A* getelementptr (%A, %A* null, i32 1) to i64

  std::vector<llvm::Value*> arg_values = {sizelp};
  return builder_.CreatePointerCast(
      builder_.CreateCall(module_->getLLVMModule()->getFunction("GC_malloc"),
                          llvm::ArrayRef<llvm::Value*>(arg_values)),
      Ty->getPointerTo(), Name);
}

bool translator::copyFull(value* src,
                          value* dest,
                          std::string const& name,
                          llvm::Value* newv,
                          value* defp)
{
  auto parent = dest->getParent();
  auto lval   = newv ? newv : dest->getLLVM();
  auto rval   = src->getLLVM();

  if (!src->isLazy()) {
    if (lval->getType()->isPointerTy()) {
      if ((src->isFundamental() ? lval->getType()->getPointerElementType() : lval->getType()) ==
          rval->getType()) {
        if (src->isFundamental()) {
          builder_.CreateStore(rval, lval);
        } else {
          std::vector<llvm::Type*> list;
          list.push_back(builder_.getInt8Ty()->getPointerTo());
          list.push_back(builder_.getInt8Ty()->getPointerTo());
          list.push_back(builder_.getInt64Ty());
          llvm::Function* fmemcpy = llvm::Intrinsic::getDeclaration(module_->getLLVMModule(),
                                                                    llvm::Intrinsic::memcpy, list);

          std::vector<llvm::Value*> arg_values;
          arg_values.push_back(builder_.CreatePointerCast(lval, builder_.getInt8PtrTy()));
          arg_values.push_back(builder_.CreatePointerCast(rval, builder_.getInt8PtrTy()));
          arg_values.push_back(sizeofType(rval->getType()));
          arg_values.push_back(builder_.getInt32(0));
          arg_values.push_back(builder_.getInt1(0));
          builder_.CreateCall(fmemcpy, llvm::ArrayRef<llvm::Value*>(arg_values));
        }
      } else {
        return false;
      }
    } else {
      return false;
    }
  }

  if (name != "") {
    (parent ? parent : (defp ? defp : thisScope_))->symbols()[name] =
        src->isLazy() ? src->copy() : src->copyWithNewLLVMValue(lval);
  }
  return true;
}

llvm::Value* translator::sizeofType(llvm::Type* ptrT)
{
  std::vector<llvm::Value*> idxList = {builder_.getInt32(1)};
  return builder_.CreatePtrToInt(
      builder_.CreateGEP(ptrT->getPointerElementType(),
                         llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptrT)),
                         idxList),
      builder_.getInt64Ty());
  // ptrtoint %A* getelementptr (%A, %A* null, i32 1) to i64
}

value* translator::apply_op(ast::binary_op<ast::add> const& op, std::vector<value*> const& args)
{
  if (std::none_of(args.begin(), args.end(),
                   [](auto const& x) { return x->getLLVM()->getType()->isDoubleTy(); }))
    return new value(builder_.CreateAdd(args[0]->getLLVM(), args[1]->getLLVM()), op);
  else
    return new value(builder_.CreateFAdd(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::sub> const& op, std::vector<value*> const& args)
{
  if (std::none_of(args.begin(), args.end(),
                   [](auto const& x) { return x->getLLVM()->getType()->isDoubleTy(); }))
    return new value(builder_.CreateSub(args[0]->getLLVM(), args[1]->getLLVM()), op);
  else
    return new value(builder_.CreateFSub(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::pow> const& op, std::vector<value*> const& args)
{
  module_->link_libraries_.push_back("m");
  std::vector<llvm::Type*> list;
  llvm::Value* lv;
  if (args[0]->getLLVM()->getType()->isDoubleTy())
    lv = args[0]->getLLVM();
  else
    lv = builder_.CreateSIToFP(args[0]->getLLVM(), builder_.getDoubleTy());

  list.push_back(lv->getType());
  llvm::Function* fpow = llvm::Intrinsic::getDeclaration(
      module_->getLLVMModule(),
      args[1]->getLLVM()->getType()->isIntegerTy() ? llvm::Intrinsic::powi : llvm::Intrinsic::pow,
      list);

  std::vector<llvm::Value*> arg_values;
  arg_values.push_back(lv);
  arg_values.push_back(args[1]->getLLVM());
  auto* res = builder_.CreateCall(fpow, llvm::ArrayRef<llvm::Value*>(arg_values));
  return new value(args[1]->getLLVM()->getType()->isIntegerTy()
                       ? builder_.CreateFPToSI(res, args[1]->getLLVM()->getType())
                       : res,
                   op);
}

value* translator::apply_op(ast::binary_op<ast::mul> const& op, std::vector<value*> const& args)
{
  if (std::none_of(args.begin(), args.end(),
                   [](auto const& x) { return x->getLLVM()->getType()->isDoubleTy(); }))
    return new value(builder_.CreateMul(args[0]->getLLVM(), args[1]->getLLVM()), op);
  else
    return new value(builder_.CreateFMul(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::div> const& op, std::vector<value*> const& args)
{
  if (std::none_of(args.begin(), args.end(),
                   [](auto const& x) { return x->getLLVM()->getType()->isDoubleTy(); }))
    return new value(builder_.CreateSDiv(args[0]->getLLVM(), args[1]->getLLVM()), op);
  else
    return new value(builder_.CreateFDiv(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::rem> const& op, std::vector<value*> const& args)
{
  if (std::none_of(args.begin(), args.end(),
                   [](auto const& x) { return x->getLLVM()->getType()->isDoubleTy(); }))
    return new value(builder_.CreateSRem(args[0]->getLLVM(), args[1]->getLLVM()), op);
  else
    return new value(builder_.CreateFRem(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::shl> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateShl(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::shr> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateLShr(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::iand> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateAnd(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::ior> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateOr(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::ixor> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateXor(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::land> const& op, std::vector<value*> const& args)
{
  return new value(
      builder_.CreateAnd(builder_.CreateICmpNE(args[0]->getLLVM(),
                                               llvm::Constant::getNullValue(builder_.getInt1Ty())),
                         builder_.CreateICmpNE(args[1]->getLLVM(),
                                               llvm::Constant::getNullValue(builder_.getInt1Ty()))),
      op);
}

value* translator::apply_op(ast::binary_op<ast::lor> const& op, std::vector<value*> const& args)
{
  return new value(
      builder_.CreateOr(builder_.CreateICmpNE(args[0]->getLLVM(),
                                              llvm::Constant::getNullValue(builder_.getInt1Ty())),
                        builder_.CreateICmpNE(args[1]->getLLVM(),
                                              llvm::Constant::getNullValue(builder_.getInt1Ty()))),
      op);
}

value* translator::apply_op(ast::binary_op<ast::eeq> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateICmpEQ(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::neq> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateICmpNE(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::gt> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateICmpSGT(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::lt> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateICmpSLT(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::gtq> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateICmpSGE(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::ltq> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateICmpSLE(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::assign> const& op, std::vector<value*> const& args)
{
  auto lval         = args[0]->getLLVM();
  auto const rval   = args[1]->getLLVM();
  auto const parent = args[0]->getParent();

  auto const n = args[0]->getName();

  if (rval->getType()->isVoidTy())
    throw error("Cannot assign the value of void type", ast::attr(op).where, errorType::Translate);

  if (!lval) {  // first appear in the block (variable declaration)
    if (parent) {
      if (parent->getLLVM()->getType()->isStructTy()) {  // structure
        // [feature/var-struct-array] Add a member to the structure, store it to
        // lval, and add args[1] to fields
        throw error("Variadic structure: This feature is not supported yet", ast::attr(op).where,
                    errorType::Translate);
      } else if (parent->getLLVM()->getType()->isArrayTy()) {  // array
        // [feature/var-struct-array] Add an element to the array, store it to
        // lval, and add args[1] to fields
        throw error("Variadic array: This feature is not supported yet", ast::attr(op).where,
                    errorType::Translate);
      } else {
        assert(false);  // unreachable
      }
    } else {  // not a member of array or structure
      assert(ast::isa<ast::variable>(ast::val(op)[0]) &&
             "Assigning to non-variable expression with no parent");
      auto va = ast::unpack<ast::variable>(ast::val(op)[0]);
      auto thety =
          args[1]->isFundamental() ? rval->getType() : rval->getType()->getPointerElementType();
      if (ast::attr(va).attributes.count("heap"))
        lval = createGCMalloc(thety, nullptr, n);
      else
        lval = builder_.CreateAlloca(thety, nullptr, n);
    }
  }

  if (!copyFull(args[1], args[0], n, lval)) {
    throw error(
        "Cannot assign to the value of incompatible type (lval: " + getNameString(lval->getType()) +
            ", rval: " + getNameString(rval->getType()) + ")",
        ast::attr(op).where, errorType::Translate);
  }
  return args[1];
}

value* translator::apply_op(ast::binary_op<ast::call> const& op, std::vector<value*> const& args)
{
  bool isadot = ast::isa<ast::binary_op<ast::adot>>(ast::val(op)[0]);
  bool isodot = ast::isa<ast::binary_op<ast::odot>>(ast::val(op)[0]) || isadot;
  ast::expr op_unpacked;
  if (isodot) {
    op_unpacked = isadot ? ast::val(ast::unpack<ast::binary_op<ast::adot>>(ast::val(op)[0]))[0]
                         : ast::val(ast::unpack<ast::binary_op<ast::odot>>(ast::val(op)[0]))[0];
  }

  assert(args[1]->isVoid());  // args[1] should be arglist

  if (args[0]->getLLVM()->getType()->isLabelTy()) {
    if (isodot) {
      throw error("Calling scope with odot operator is not allowed", ast::attr(op).where,
                  errorType::Translate);
    } else if (!ast::val(ast::unpack<ast::arglist>(ast::val(op)[1])).empty()) {
      throw error("Calling scope with arguments is not allowed", ast::attr(op).where,
                  errorType::Translate);
    } else {
      return evaluate(args[0], std::vector<value*>{}, *this);
    }
  } else {
    llvm::Value* tocall;
    std::vector<llvm::Value*> arg_values;
    ret_table_t* ret_table = nullptr;

    auto arglist = ast::unpack<ast::arglist>(ast::val(op)[1]);

    if (!args[0]->isLazy()) {
      tocall = args[0]->getLLVM();
      if (!tocall->getType()->isPointerTy()) {
        throw error(
            "Cannot call a non-pointer value (type: " + getNameString(tocall->getType()) + ")",
            ast::attr(op).where, errorType::Translate);
      } else if (!tocall->getType()->getPointerElementType()->isFunctionTy()) {
        throw error("Cannot call a value which is not a function pointer  (type: " +
                        getNameString(tocall->getType()) + ")",
                    ast::attr(op).where, errorType::Translate);
      } else {
        auto functy = tocall->getType()->getPointerElementType();
        if (functy->getFunctionNumParams() != ast::val(arglist).size() &&
            !functy->isFunctionVarArg()) {
          throw error("The number of arguments doesn't match: required " +
                          std::to_string(functy->getFunctionNumParams()) + " but supplied " +
                          std::to_string(ast::val(arglist).size()),
                      ast::attr(op).where, errorType::Translate);
        }
        for (auto const arg : ast::val(arglist) | boost::adaptors::indexed()) {
          auto rv = boost::apply_visitor(*this, arg.value());
          if (rv->isLazy()) {
            throw error("Cannot pass a lazy value to c-style functions", ast::attr(op).where,
                        errorType::Translate);
          } else if (functy->getFunctionParamType(static_cast<unsigned>(arg.index())) !=
                         rv->getLLVM()->getType() &&
                     !functy->isFunctionVarArg()) {
            throw error("Type mismatch on argument No." + std::to_string(arg.index()) +
                            ": expected \"" +
                            getNameString(
                                functy->getFunctionParamType(static_cast<unsigned>(arg.index()))) +
                            "\" but supplied \"" + getNameString(rv->getLLVM()->getType()) + "\"",
                        ast::attr(op).where, errorType::Translate);
          } else {
            arg_values.push_back(rv->getLLVM());
          }
        }
        if (isodot)
          arg_values.push_back(args[0]->getParent()->getLLVM());
      }
    } else {
      std::vector<value*> vary;
      for (auto const& argast : ast::val(arglist)) {
        auto rv = boost::apply_visitor(*this, argast);
        vary.push_back(rv);
        if (!rv->isLazy())
          arg_values.push_back(rv->getLLVM());
      }
      if (isodot) {
        auto ob_parent = boost::apply_visitor(*this, op_unpacked);
        vary.push_back(ob_parent);
        arg_values.push_back(ob_parent->getLLVM());
      }

      auto v    = evaluate(args[0], vary, *this);
      tocall    = v->getLLVM();
      ret_table = v->getRetTable();
    }

    auto destv =
        new value(builder_.CreateCall(tocall, llvm::ArrayRef<llvm::Value*>(arg_values)), op);
    if (ret_table)
      destv->applyRetTable(ret_table);

    if (isadot && !ast::attr(op).survey) {
      auto lvaled               = ast::set_lval(op_unpacked, true);
      std::vector<value*> argvs = {boost::apply_visitor(*this, lvaled), destv};
      /* constructing ast::binary_op with dummy arguments */
      return apply_op(ast::set_where(ast::binary_op<ast::assign>{{op, op}}, ast::attr(op).where),
                      argvs);
    }

    return destv;
  }
}

value* translator::apply_op(ast::binary_op<ast::at> const& op, std::vector<value*> const& args)
{
  auto r    = args[1]->getLLVM();
  auto rval = r->getType()->isPointerTy() ? builder_.CreateLoad(r) : r;

  auto lval = args[0]->getLLVM();

  if (!rval->getType()->isIntegerTy()) {
    throw error("Array's index must be integer, not " + getNameString(rval->getType()),
                ast::attr(op).where, errorType::Translate);
  }
  if (!lval->getType()->isPointerTy()) {
    throw error("Cannot get element from non-pointer type " + getNameString(lval->getType()),
                ast::attr(op).where, errorType::Translate);
  }

  if (llvm::isa<llvm::ConstantInt>(args[1]->getLLVM()) &&
      lval->getType()->getPointerElementType()->isArrayTy()) {
    uint32_t aindex =
        static_cast<uint32_t>(llvm::cast<llvm::ConstantInt>(args[1]->getLLVM())
                                  ->getValue()
                                  .getLimitedValue(std::numeric_limits<uint32_t>::max()));
    std::string istr = std::to_string(aindex);
    auto it          = args[0]->symbols().find(istr);
    if (it == args[0]->symbols().end()) {
      throw error("Index " + std::to_string(aindex) + " is out of range.", ast::attr(op).where,
                  errorType::Translate);
    }
    auto ep = it->second;

    if (!ep->isLazy()) {
      std::vector<llvm::Value*> idxList = {builder_.getInt32(0), builder_.getInt32(aindex)};
      auto p = builder_.CreateInBoundsGEP(lval->getType()->getPointerElementType(), lval,
                                          llvm::ArrayRef<llvm::Value*>(idxList));
      ep->setLLVM(p);
    }
    ep->setName(istr);

    if (ast::attr(op).lval || ep->isLazy() || !ep->isFundamental())
      return ep->copy();
    else
      return ep->copyWithNewLLVMValue(builder_.CreateLoad(ep->getLLVM()));
  } else {  // specifing index with non-constant integer or pointer to pointer
    llvm::Value* ep;

    if (lval->getType()->getPointerElementType()->isArrayTy()) {
      if (args[0]->symbols().begin()->second->isLazy()) {
        throw error(
            "Getting value from an array which contains lazy value with an index "
            "of non-constant value",
            ast::attr(op).where, errorType::Translate);
      }

      std::vector<llvm::Value*> idxList = {builder_.getInt32(0), rval};
      ep = builder_.CreateInBoundsGEP(lval->getType()->getPointerElementType(), lval,
                                      llvm::ArrayRef<llvm::Value*>(idxList));
    } else {
      if (!llvm::GetElementPtrInst::getIndexedType(lval->getType()->getPointerElementType(), rval))
        throw error("Cannot get element from incompatible type " + getNameString(lval->getType()),
                    ast::attr(op).where, errorType::Translate);
      ep = builder_.CreateGEP(lval->getType()->getPointerElementType(), lval, rval);
    }

    if (ast::attr(op).lval || ep->getType()->getPointerElementType()->isStructTy() ||
        ep->getType()->getPointerElementType()->isArrayTy())
      return new value(ep, op);
    else
      return new value(builder_.CreateLoad(ep), op);
  }
}

value* translator::apply_op(ast::binary_op<ast::dot> const& op, std::vector<value*> const& args)
{
  auto lval = args[0]->getLLVM();

  assert(ast::isa<ast::struct_key>(ast::val(op)[1]) && "rhs of dot operator must be a struct key");
  auto id = ast::val(ast::unpack<ast::struct_key>(ast::val(op)[1]));

  if (!lval->getType()->isPointerTy())
    throw error("Cannot get \"" + id + "\" from non-pointer type " + getNameString(lval->getType()),
                ast::attr(op).where, errorType::Translate);

  lval = lval->getType()->getPointerElementType()->isPointerTy() ? builder_.CreateLoad(lval) : lval;

  if (!lval->getType()->getPointerElementType()->isStructTy())
    throw error(
        "Cannot get \"" + id + "\" from non-structure type " + getNameString(lval->getType()),
        ast::attr(op).where, errorType::Translate);

  auto elm = args[0]->symbols().find(id);

  if (elm == args[0]->symbols().end()) {
    throw error("No member named \"" + id + "\" in the structure", ast::attr(op).where,
                errorType::Translate);
  }

  if (!elm->second->isLazy()) {
    auto ptr = builder_.CreateStructGEP(lval->getType()->getPointerElementType(), lval,
                                        args[0]->fields()[id]);
    elm->second->setLLVM(ptr);
  }
  elm->second->setName(id);

  if (ast::attr(op).lval || elm->second->isLazy() || !elm->second->isFundamental())
    return elm->second->copy();
  else
    return elm->second->copyWithNewLLVMValue(builder_.CreateLoad(elm->second->getLLVM()));
}

value* translator::apply_op(ast::binary_op<ast::odot> const& op, std::vector<value*> const& args)
{
  if (!ast::attr(op).to_call)
    throw error("Objective dot operator without call operator", ast::attr(op).where,
                errorType::Translate);
  return apply_op(ast::set_where(ast::binary_op<ast::dot>({ast::val(op)[0], ast::val(op)[1]}),
                                 ast::attr(op).where),
                  args);
}

value* translator::apply_op(ast::binary_op<ast::adot> const& op, std::vector<value*> const& args)
{
  if (!ast::attr(op).to_call)
    throw error("Objective dot operator without call operator", ast::attr(op).where,
                errorType::Translate);
  return apply_op(ast::set_where(ast::binary_op<ast::dot>({ast::val(op)[0], ast::val(op)[1]}),
                                 ast::attr(op).where),
                  args);
}

value* translator::apply_op(ast::single_op<ast::ret> const& op, std::vector<value*> const& args)
{
  builder_.CreateRet(args[0]->getLLVM());
  auto newv = new value();
  newv->setRetTable(args[0]->generateRetTable());
  return newv;  // void+rettable
}

value* translator::apply_op(ast::single_op<ast::lnot> const& op, std::vector<value*> const& args)
{
  return new value(
      builder_.CreateXor(builder_.CreateICmpNE(args[0]->getLLVM(),
                                               llvm::Constant::getNullValue(builder_.getInt1Ty())),
                         builder_.getInt32(1)),
      op);
}

value* translator::apply_op(ast::single_op<ast::inot> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateXor(args[0]->getLLVM(), builder_.getInt32(1)), op);
}

value* translator::apply_op(ast::single_op<ast::inc> const& op, std::vector<value*> const& args)
{
  assert(false);  // unreachable
}

value* translator::apply_op(ast::single_op<ast::dec> const& op, std::vector<value*> const& args)
{
  assert(false);  // unreachable
}

value* translator::apply_op(ast::ternary_op<ast::cond> const& op, std::vector<value*> const& args)
{
  if (args[0]->isLazy())
    throw error("Conditional operator with lazy value is not supported", ast::attr(op).where,
                errorType::Translate);

  if (args[1]->getLLVM()->getType() != args[2]->getLLVM()->getType())
    throw error("Conditional operator with incompatible value types (lhs: " +
                    getNameString(args[1]->getLLVM()->getType()) +
                    ", rhs: " + getNameString(args[2]->getLLVM()->getType()) + ")",
                ast::attr(op).where, errorType::Translate);

  if (args[1]->getLLVM()->getType()->isLabelTy()) {
    llvm::BasicBlock* thenbb =
        llvm::BasicBlock::Create(module_->getContext(), "", builder_.GetInsertBlock()->getParent());
    llvm::BasicBlock* elsebb =
        llvm::BasicBlock::Create(module_->getContext(), "", builder_.GetInsertBlock()->getParent());

    auto pb = builder_.GetInsertBlock();
    auto pp = builder_.GetInsertPoint();

    auto prevScope = thisScope_;

    llvm::BasicBlock* mergebb =
        llvm::BasicBlock::Create(module_->getContext(), "", builder_.GetInsertBlock()->getParent());

    bool mergebbShouldBeErased = true;

    assert(ast::isa<ast::scope>(args[1]->getAst()) && ast::isa<ast::scope>(args[2]->getAst()) &&
           "Applying non-scope value as scope");
    auto secondsc = ast::unpack<ast::scope>(args[1]->getAst());
    auto thirdsc  = ast::unpack<ast::scope>(args[2]->getAst());

    builder_.SetInsertPoint(thenbb);
    thisScope_ = args[1];
    if (apply_bb(secondsc, *this).first) {
      builder_.CreateBr(mergebb);
      mergebbShouldBeErased &= false;
    }

    builder_.SetInsertPoint(elsebb);
    thisScope_ = args[2];
    if (apply_bb(thirdsc, *this).first) {
      builder_.CreateBr(mergebb);
      mergebbShouldBeErased &= false;
    }

    thisScope_ = prevScope;

    if (mergebbShouldBeErased)
      mergebb->eraseFromParent();

    builder_.SetInsertPoint(pb, pp);

    builder_.CreateCondBr(args[0]->getLLVM(), thenbb, elsebb);

    if (!mergebbShouldBeErased)
      builder_.SetInsertPoint(mergebb);

    return new value();  // Void
  } else {
    if (args[1]->isLazy() || args[2]->isLazy())
      throw error("Conditional operator with lazy value is currently not supported",
                  ast::attr(op).where, errorType::Translate);

    if (!std::equal(args[1]->fields().begin(), args[1]->fields().end(), args[2]->fields().begin(),
                    args[2]->fields().end(), [](auto& s, auto& t) { return s.first == t.first; })) {
      throw error("Conditional operator with different fields", ast::attr(op).where,
                  errorType::Translate);
    }

    auto pb = builder_.GetInsertBlock();
    auto pp = builder_.GetInsertPoint();

    auto destlv =
        builder_.CreateAlloca(ast::attr(op).lval ? args[1]->getLLVM()->getType()->getPointerTo()
                                                 : args[1]->getLLVM()->getType());

    llvm::BasicBlock* thenbb =
        llvm::BasicBlock::Create(module_->getContext(), "", builder_.GetInsertBlock()->getParent());
    llvm::BasicBlock* elsebb =
        llvm::BasicBlock::Create(module_->getContext(), "", builder_.GetInsertBlock()->getParent());
    llvm::BasicBlock* mergebb =
        llvm::BasicBlock::Create(module_->getContext(), "", builder_.GetInsertBlock()->getParent());

    auto lvaled_then = ast::set_lval(ast::val(op)[1], true);
    auto lvaled_else = ast::set_lval(ast::val(op)[2], true);
    value* thenv     = ast::attr(op).lval ? boost::apply_visitor(*this, lvaled_then) : args[1];
    value* elsev     = ast::attr(op).lval ? boost::apply_visitor(*this, lvaled_else) : args[2];

    builder_.SetInsertPoint(thenbb);
    builder_.CreateStore(thenv->getLLVM(), destlv);
    builder_.CreateBr(mergebb);
    builder_.SetInsertPoint(elsebb);
    builder_.CreateStore(elsev->getLLVM(), destlv);
    builder_.CreateBr(mergebb);
    builder_.SetInsertPoint(pb, pp);
    builder_.CreateCondBr(args[0]->getLLVM(), thenbb, elsebb);

    builder_.SetInsertPoint(mergebb);

    auto destv       = new value(builder_.CreateLoad(destlv), op);
    destv->symbols() = args[1]->symbols();
    destv->fields()  = args[1]->fields();
    return destv;
  }
}

};  // namespace assembly
};  // namespace scopion
