#include "scopion/assembly/evaluator.hpp"
#include "scopion/assembly/translator.hpp"
#include "scopion/assembly/value.hpp"

#include "scopion/error.hpp"

#include <boost/range/adaptor/indexed.hpp>

#include <algorithm>
#include <iostream>

#include <csignal>

namespace scopion
{
namespace assembly
{
llvm::Value* translator::createGCMalloc(llvm::Type* Ty,
                                        llvm::Value* ArraySize,
                                        const llvm::Twine& Name)
{
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
  return builder_.CreatePointerCast(builder_.CreateCall(module_->getFunction("GC_malloc"),
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
          list.push_back(builder_.getInt32Ty());
          list.push_back(builder_.getInt1Ty());
          llvm::Function* fmemcpy =
              llvm::Intrinsic::getDeclaration(module_.get(), llvm::Intrinsic::memcpy, list);

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
  return new value(builder_.CreateAdd(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::sub> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateSub(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::mul> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateMul(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::div> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateSDiv(args[0]->getLLVM(), args[1]->getLLVM()), op);
}

value* translator::apply_op(ast::binary_op<ast::rem> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateSRem(args[0]->getLLVM(), args[1]->getLLVM()), op);
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
  if (!lval) {  // first appear in the block (variable declaration)
    if (parent) {
      if (parent->getLLVM()->getType()->isStructTy()) {  // structure
        // [feature/var-struct-array] Add a member to the structure, store it to
        // lval, and add args[1] to fields
        throw error("Variadic structure: This feature is not supported yet", ast::attr(op).where,
                    code_range_);
      } else if (parent->getLLVM()->getType()->isArrayTy()) {  // array
        // [feature/var-struct-array] Add an element to the array, store it to
        // lval, and add args[1] to fields
        throw error("Variadic array: This feature is not supported yet", ast::attr(op).where,
                    code_range_);
      } else {
        assert(false);  // unreachable
      }
    } else {  // not a member of array or structure
      assert(ast::val(op)[0].type() == typeid(ast::value) &&
             "Assigning to operator expression with no parent");
      if (rval->getType()->isVoidTy())
        throw error("Cannot assign the value of void type", ast::attr(op).where, code_range_);
      lval = createGCMalloc(
          args[1]->isFundamental() ? rval->getType() : rval->getType()->getPointerElementType(),
          nullptr, n);
    }
  }

  if (!copyFull(args[1], args[0], n, lval)) {
    throw error(
        "Cannot assign to the value of incompatible type (" + getNameString(lval->getType()) + ")",
        ast::attr(op).where, code_range_);
  }
  return args[1];
}

value* translator::apply_op(ast::binary_op<ast::call> const& op, std::vector<value*> const& args)
{
  bool isodot = true;
  try {
    ast::unpack<ast::binary_op<ast::odot>>(ast::val(op)[0]);
  } catch (boost::bad_get&) {
    isodot = false;
  }
  assert(args[1]->isVoid());  // args[1] should be arglist

  if (args[0]->getLLVM()->getType()->isLabelTy()) {
    if (isodot) {
      throw error("Calling scope with odot operator is not allowed", ast::attr(op).where,
                  code_range_);
    } else if (!ast::val(ast::unpack<ast::arglist>(ast::val(op)[1])).empty()) {
      throw error("Calling scope with arguments is not allowed", ast::attr(op).where, code_range_);
    } else {
      return evaluate(args[0], std::vector<value*>{}, *this);
    }
  } else {
    llvm::Value* tocall;
    std::vector<llvm::Value*> arg_values;

    auto arglist = ast::unpack<ast::arglist>(ast::val(op)[1]);

    if (!args[0]->isLazy()) {
      tocall = args[0]->getLLVM();
      if (!tocall->getType()->isPointerTy()) {
        throw error(
            "Cannot call a non-pointer value (type: " + getNameString(tocall->getType()) + ")",
            ast::attr(op).where, code_range_);
      } else if (!tocall->getType()->getPointerElementType()->isFunctionTy()) {
        throw error("Cannot call a value which is not a function pointer  (type: " +
                        getNameString(tocall->getType()) + ")",
                    ast::attr(op).where, code_range_);
      } else {
        std::transform(ast::val(arglist).begin(), ast::val(arglist).end(),
                       std::back_inserter(arg_values), [this, &op](auto& x) {
                         auto rv = boost::apply_visitor(*this, x);
                         if (rv->isLazy()) {
                           throw error("Cannot pass a lazy value to c-style functions",
                                       ast::attr(op).where, code_range_);
                         } else {
                           return rv->getLLVM();
                         }
                       });
        if (isodot)
          arg_values.push_back(args[0]->getParent()->getLLVM());
      }
    } else {
      if (ast::attr(op).survey) {
        tocall = args[0]->getLLVM();
      } else {
        std::vector<value*> vary;
        for (auto const& argast : ast::val(arglist)) {
          auto rv = boost::apply_visitor(*this, argast);
          vary.push_back(rv);
          if (!rv->isLazy())
            arg_values.push_back(rv->getLLVM());
        }
        if (isodot) {
          auto p = boost::apply_visitor(
              *this, ast::val(ast::unpack<ast::binary_op<ast::odot>>(ast::val(op)[0]))[0]);
          vary.push_back(p);
          arg_values.push_back(p->getLLVM());
        }

        tocall = evaluate(args[0], vary, *this)->getLLVM();
      }
    }

    return new value(builder_.CreateCall(tocall, llvm::ArrayRef<llvm::Value*>(arg_values)), op);
  }
}

value* translator::apply_op(ast::binary_op<ast::at> const& op, std::vector<value*> const& args)
{
  auto r    = args[1]->getLLVM();
  auto rval = r->getType()->isPointerTy() ? builder_.CreateLoad(r) : r;

  auto lval = args[0]->getLLVM();

  if (!rval->getType()->isIntegerTy()) {
    throw error("Array's index must be integer, not " + getNameString(rval->getType()),
                ast::attr(op).where, code_range_);
  }
  if (!lval->getType()->isPointerTy()) {
    throw error("Cannot get element from non-pointer type " + getNameString(lval->getType()),
                ast::attr(op).where, code_range_);
  }

  if (!lval->getType()->getPointerElementType()->isArrayTy()) {
    lval = builder_.CreateLoad(lval);

    if (!lval->getType()->getPointerElementType()->isArrayTy()) {
      throw error("Cannot get element from non-array type " + getNameString(lval->getType()),
                  ast::attr(op).where, code_range_);
    }
  }
  // Now lval's type is pointer to array

  try {
    int aindex       = ast::val(ast::unpack<ast::integer>(ast::val(op)[1]));
    std::string istr = std::to_string(aindex);
    try {
      auto ep = args[0]->symbols().at(istr);

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
    } catch (std::out_of_range&) {
      throw error("Index " + std::to_string(aindex) + " is out of range.", ast::attr(op).where,
                  code_range_);
    }
  } catch (boost::bad_get&) {  // specifing index with non-literal integer
    if (!args[0]->symbols().begin()->second->isLazy()) {  // No Lazy is in it
      std::vector<llvm::Value*> idxList = {builder_.getInt32(0), rval};
      auto ep = builder_.CreateInBoundsGEP(lval->getType()->getPointerElementType(), lval,
                                           llvm::ArrayRef<llvm::Value*>(idxList));

      if (ast::attr(op).lval || ep->getType()->getPointerElementType()->isStructTy() ||
          ep->getType()->getPointerElementType()->isArrayTy())
        return new value(ep, op);
      else
        return new value(builder_.CreateLoad(ep), op);
    } else {  // contains lazy
      throw error(
          "Getting value from an array which contains lazy value with an index "
          "of non-conpiletime value",
          ast::attr(op).where, code_range_);
    }
  }
}

value* translator::apply_op(ast::binary_op<ast::dot> const& op, std::vector<value*> const& args)
{
  auto lval = args[0]->getLLVM();

  auto id = ast::val(ast::unpack<ast::identifier>(ast::val(op)[1]));

  if (!lval->getType()->isPointerTy())
    throw error("Cannot get \"" + id + "\" from non-pointer type " + getNameString(lval->getType()),
                ast::attr(op).where, code_range_);

  lval = lval->getType()->getPointerElementType()->isPointerTy() ? builder_.CreateLoad(lval) : lval;

  if (!lval->getType()->getPointerElementType()->isStructTy())
    throw error(
        "Cannot get \"" + id + "\" from non-structure type " + getNameString(lval->getType()),
        ast::attr(op).where, code_range_);

  auto elm = args[0]->symbols().find(id);

  if (elm == args[0]->symbols().end()) {
    throw error("No member named \"" + id + "\" in the structure", ast::attr(op).where,
                code_range_);
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
    throw error("Objective dot operator without call operator", ast::attr(op).where, code_range_);
  return apply_op(ast::binary_op<ast::dot>({ast::val(op)[0], ast::val(op)[1]}), args);
}

value* translator::apply_op(ast::single_op<ast::ret> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateRet(args[0]->getLLVM()), op);
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
  return new value(builder_.CreateAdd(args[0]->getLLVM(), builder_.getInt32(1)), op);
}

value* translator::apply_op(ast::single_op<ast::dec> const& op, std::vector<value*> const& args)
{
  return new value(builder_.CreateSub(args[0]->getLLVM(), builder_.getInt32(1)), op);
}

value* translator::apply_op(ast::ternary_op<ast::cond> const& op, std::vector<value*> const& args)
{
  if (!args[1]->getLLVM()->getType()->isLabelTy() || !args[2]->getLLVM()->getType()->isLabelTy())
    throw error("The arguments of ternary operator should be a block", ast::attr(op).where,
                code_range_);

  llvm::BasicBlock* thenbb =
      llvm::BasicBlock::Create(module_->getContext(), "", builder_.GetInsertBlock()->getParent());
  llvm::BasicBlock* elsebb =
      llvm::BasicBlock::Create(module_->getContext(), "", builder_.GetInsertBlock()->getParent());

  auto pb = builder_.GetInsertBlock();
  auto pp = builder_.GetInsertPoint();

  llvm::BasicBlock* mergebb =
      llvm::BasicBlock::Create(module_->getContext(), "", builder_.GetInsertBlock()->getParent());

  bool mergebbShouldBeErased = true;

  auto prevScope = thisScope_;

  ast::scope secondsc;
  ast::scope thirdsc;
  try {
    secondsc = ast::unpack<ast::scope>(args[1]->getAst());
    thirdsc  = ast::unpack<ast::scope>(args[2]->getAst());
  } catch (boost::bad_get&) {
    throw error("Applying non-scope value as scope", ast::attr(op).where, code_range_);
  }

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

  if (args[0]->isLazy())
    throw error("Conditions with lazy value is not supported", ast::attr(op).where, code_range_);

  builder_.CreateCondBr(args[0]->getLLVM(), thenbb, elsebb);

  if (!mergebbShouldBeErased)
    builder_.SetInsertPoint(mergebb);

  return new value();  // Void
}

};  // namespace assembly
};  // namespace scopion
