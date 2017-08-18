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
  auto lval       = args[0]->getLLVM();
  auto const rval = args[1]->getLLVM();
  if (!lval) {  // first appear in the block (variable declaration)
    auto const parent = args[0]->getParent();
    if (!parent) {  // not a member of array or structure
      assert(ast::val(op)[0].type() == typeid(ast::value) &&
             "Assigning to operator expression with no parent");
      auto lvar = ast::unpack<ast::variable>(ast::val(op)[0]);  // auto6 lvar = not working
      if (rval->getType()->isVoidTy())
        throw error("Cannot assign the value of void type", ast::attr(op).where, code_range_);

      if (!args[1]->isLazy())
        args[1]->setLLVM(lval = builder_.CreateAlloca(rval->getType(), nullptr, ast::val(lvar)));
      thisScope_->symbols()[ast::val(lvar)] = args[1];
    } else {
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
    }
  } else {
    if (args[1]->isLazy()) {
      if (auto parent = args[0]->getParent()) {
        // not good way...?
        // ow? args[1]->setParent(parent);
        parent->symbols()[ast::val(ast::unpack<ast::identifier>(
            ast::val(ast::unpack<ast::op<ast::dot, 2>>(ast::val(op)[0]))[1]))] = args[1];
      } else {
        thisScope_->symbols()[ast::val(ast::unpack<ast::variable>(ast::val(op)[0]))] = args[1];
      }
    }
  }

  if (!args[1]->isLazy()) {
    if (lval->getType()->isPointerTy()) {
      if (lval->getType()->getPointerElementType() == rval->getType()) {
        builder_.CreateStore(rval, lval);
      } else {
        throw error("Cannot assign to different type of value (assigning " +
                        getNameString(rval->getType()) + " into " +
                        getNameString(lval->getType()->getPointerElementType()) + ")",
                    ast::attr(op).where, code_range_);
      }
    } else {
      throw error("Cannot assign to non-pointer value (" + getNameString(lval->getType()) + ")",
                  ast::attr(op).where, code_range_);
    }
  }
  return args[1];
}

value* translator::apply_op(ast::binary_op<ast::call> const& op, std::vector<value*> const& args)
{
  assert(args[1]->isVoid());  // args[1] should be arglist

  if (args[0]->getLLVM()->getType()->isLabelTy()) {
    if (!ast::val(ast::unpack<ast::arglist>(ast::val(op)[1])).empty()) {
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
        throw error("Cannot call a non-pointer value", ast::attr(op).where, code_range_);
      } else if (!tocall->getType()->getPointerElementType()->isFunctionTy()) {
        throw error("Cannot call a value which is not a function pointer", ast::attr(op).where,
                    code_range_);
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
    int aindex = ast::val(ast::unpack<ast::integer>(ast::val(op)[1]));
    try {
      auto ep = args[0]->symbols().at(std::to_string(aindex));
      return ast::attr(op).lval ? ep : ep->copyWithNewLLVMValue(builder_.CreateLoad(ep->getLLVM()));
    } catch (std::out_of_range&) {
      throw error("Index " + std::to_string(aindex) + " is out of range.", ast::attr(op).where,
                  code_range_);
    }
  } catch (boost::bad_get&) {  // specifing index with non-literal integer
    if (std::none_of(args[0]->symbols().begin(), args[0]->symbols().end(),
                     [](auto& x) { return x.second->isLazy(); })) {  // No Lazy is in it
      std::vector<llvm::Value*> idxList = {builder_.getInt32(0), rval};
      auto ep = builder_.CreateInBoundsGEP(lval->getType()->getPointerElementType(), lval,
                                           llvm::ArrayRef<llvm::Value*>(idxList));
      return ast::attr(op).lval ? new value(ep, op) : new value(builder_.CreateLoad(ep), op);
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
  // auto ptr = builder_.CreateStructGEP(
  //     lval->getType()->getPointerElementType(), lval,
  //     static_cast<uint32_t>(elm.first));
  // maybe useless

  if (ast::attr(op).lval || elm->second->isLazy())
    return elm->second;
  else
    return elm->second->copyWithNewLLVMValue(builder_.CreateLoad(elm->second->getLLVM()));
}

[[deprecated]] value* translator::apply_op(ast::single_op<ast::load> const& op,
                                           std::vector<value*> const& args)
{
  auto vv = args[0]->getLLVM();
  if (!vv->getType()->isPointerTy())
    throw error("Cannot load from non-pointer type " + getNameString(vv->getType()),
                ast::attr(op).where, code_range_);
  return new value(builder_.CreateLoad(vv), op);  // not good but i don't matter
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
  if (apply_bb(secondsc, *this)) {
    builder_.CreateBr(mergebb);
    mergebbShouldBeErased &= false;
  }

  builder_.SetInsertPoint(elsebb);
  thisScope_ = args[2];
  if (apply_bb(thirdsc, *this)) {
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
