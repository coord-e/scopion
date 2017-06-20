#include "scopion/assembly/scoped_value.hpp"
#include "scopion/assembly/translator.hpp"

#include "scopion/error.hpp"

#include <boost/range/adaptor/indexed.hpp>

namespace scopion {
namespace assembly {

scoped_value * translator::apply_op(ast::binary_op<ast::add> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(builder_.CreateAdd(lhs->getValue(), rhs->getValue()));
}

scoped_value * translator::apply_op(ast::binary_op<ast::sub> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(builder_.CreateSub(lhs->getValue(), rhs->getValue()));
}

scoped_value * translator::apply_op(ast::binary_op<ast::mul> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(builder_.CreateMul(lhs->getValue(), rhs->getValue()));
}

scoped_value * translator::apply_op(ast::binary_op<ast::div> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(
      builder_.CreateSDiv(lhs->getValue(), rhs->getValue()));
}

scoped_value * translator::apply_op(ast::binary_op<ast::rem> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(
      builder_.CreateSRem(lhs->getValue(), rhs->getValue()));
}

scoped_value * translator::apply_op(ast::binary_op<ast::shl> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(builder_.CreateShl(lhs->getValue(), rhs->getValue()));
}

scoped_value * translator::apply_op(ast::binary_op<ast::shr> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(
      builder_.CreateLShr(lhs->getValue(), rhs->getValue()));
}

scoped_value * translator::apply_op(ast::binary_op<ast::iand> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(builder_.CreateAnd(lhs->getValue(), rhs->getValue()));
}

scoped_value * translator::apply_op(ast::binary_op<ast::ior> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(builder_.CreateOr(lhs->getValue(), rhs->getValue()));
}

scoped_value * translator::apply_op(ast::binary_op<ast::ixor> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(builder_.CreateXor(lhs->getValue(), rhs->getValue()));
}

scoped_value * translator::apply_op(ast::binary_op<ast::land> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(builder_.CreateAnd(
      builder_.CreateICmpNE(lhs->getValue(),
                            llvm::Constant::getNullValue(builder_.getInt1Ty())),
      builder_.CreateICmpNE(rhs->getValue(), llvm::Constant::getNullValue(
                                                 builder_.getInt1Ty()))));
}

scoped_value * translator::apply_op(ast::binary_op<ast::lor> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(builder_.CreateOr(
      builder_.CreateICmpNE(lhs->getValue(),
                            llvm::Constant::getNullValue(builder_.getInt1Ty())),
      builder_.CreateICmpNE(rhs->getValue(), llvm::Constant::getNullValue(
                                                 builder_.getInt1Ty()))));
}

scoped_value * translator::apply_op(ast::binary_op<ast::eeq> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(
      builder_.CreateICmpEQ(lhs->getValue(), rhs->getValue()));
}

scoped_value * translator::apply_op(ast::binary_op<ast::neq> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(
      builder_.CreateICmpNE(lhs->getValue(), rhs->getValue()));
}

scoped_value * translator::apply_op(ast::binary_op<ast::gt> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(
      builder_.CreateICmpSGT(lhs->getValue(), rhs->getValue()));
}

scoped_value * translator::apply_op(ast::binary_op<ast::lt> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(
      builder_.CreateICmpSLT(lhs->getValue(), rhs->getValue()));
}

scoped_value * translator::apply_op(ast::binary_op<ast::gtq> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(
      builder_.CreateICmpSGE(lhs->getValue(), rhs->getValue()));
}

scoped_value * translator::apply_op(ast::binary_op<ast::ltq> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  return new scoped_value(
      builder_.CreateICmpSLE(lhs->getValue(), rhs->getValue()));
}

scoped_value * translator::apply_op(ast::binary_op<ast::assign> const &op,
                              scoped_value * const lhs, scoped_value * rhs) {
  if (!lhs) { // first appear in the block (only variable)
    auto &&lvar = boost::get<ast::variable>(boost::get<ast::value>(op.lhs));
    if (rhs->hasBlock()) {
      currentScope_->symbols[ast::val(lvar)] = rhs; // aliasing
      return rhs;
    }
    if (rhs->getType()->isVoidTy())
      throw error("Cannot assign the value of void type", ast::attr(op).where,
                  code_range_);
    auto lhsa = builder_.CreateAlloca(rhs->getType(), nullptr, ast::val(lvar));
    currentScope_->symbols[ast::val(lvar)] = new scoped_value{lhsa};
    builder_.CreateStore(rhs->getValue(), lhsa);
    return new scoped_value(builder_.CreateLoad(lhsa));
  } else {
    if (lhs->getType()->isPointerTy()) {
      if (lhs->getType()->getPointerElementType() == rhs->getType()) {
        builder_.CreateStore(rhs->getValue(), lhs->getValue());
      } else {
        throw error("Cannot assign to different type of value (assigning " +
                        getNameString(rhs->getType()) + " into " +
                        getNameString(lhs->getType()->getPointerElementType()) +
                        ")",
                    ast::attr(op).where, code_range_);
      }
    } else {
      throw error("Cannot assign to non-pointer value (" +
                      getNameString(lhs->getType()) + ")",
                  ast::attr(op).where, code_range_);
    }
    return new scoped_value(builder_.CreateLoad(lhs->getValue()));
  }
}

scoped_value * translator::apply_op(ast::binary_op<ast::call> const &op, scoped_value * lhs,
                              scoped_value * const rhs) {
  if (lhs->hasBlock()) {

    auto pb = builder_.GetInsertBlock();
    auto pp = builder_.GetInsertPoint();

    auto nb = llvm::BasicBlock::Create(module_->getContext(), "",
                                       builder_.GetInsertBlock()->getParent());

    auto theblock = llvm::BasicBlock::Create(
        module_->getContext(), "", builder_.GetInsertBlock()->getParent());
    builder_.SetInsertPoint(theblock);
    lhs->setBlock(theblock);
    auto prevScope = currentScope_;
    currentScope_ = lhs;
    for (auto const &i : *(lhs->getInsts())) {
      boost::apply_visitor(*this, i);
    }

    builder_.CreateBr(nb);
    currentScope_ = prevScope;

    builder_.SetInsertPoint(pb, pp);

    builder_.CreateBr(theblock);

    builder_.SetInsertPoint(nb);

    return new scoped_value(); // Voidをかえしたいんだけど
  }

  if (!lhs->getType()->isPointerTy())
    throw error("Cannot call function from non-pointer type " +
                    getNameString(lhs->getType()),
                ast::attr(op).where, code_range_);

  if (!lhs->getType()->getPointerElementType()->isFunctionTy())
    throw error("Cannot call function from non-function pointer type " +
                    getNameString(lhs->getType()),
                ast::attr(op).where, code_range_);

  auto &&arglist = boost::get<ast::arglist>(boost::get<ast::value>(op.rhs));
  if (lhs->getType()->getPointerElementType()->getFunctionNumParams() !=
      ast::val(arglist).size())
    throw error("The number of arguments doesn't match: required " +
                    std::to_string(lhs->getType()
                                       ->getPointerElementType()
                                       ->getFunctionNumParams()) +
                    " but supplied " + std::to_string(ast::val(arglist).size()),
                ast::attr(op).where, code_range_);
  std::vector<llvm::Value *> args;
  for (auto const &arg : ast::val(arglist) | boost::adaptors::indexed()) {
    auto v = boost::apply_visitor(*this, arg.value());
    auto expected =
        lhs->getType()->getPointerElementType()->getFunctionParamType(
            arg.index());
    if (v->getType() != expected)
      throw error("The type of argument " + std::to_string(arg.index() + 1) +
                      " doesn't match: expected " + getNameString(expected) +
                      " but " + getNameString(v->getType()),
                  ast::attr(op).where, code_range_);

    args.push_back(v->getValue());
  }

  return new scoped_value(builder_.CreateCall(
      lhs->getValue(), llvm::ArrayRef<llvm::Value *>(args)));
}

scoped_value * translator::apply_op(ast::binary_op<ast::at> const &op,
                              scoped_value * const lhs, scoped_value * const rhs) {
  auto rval = rhs->getType()->isPointerTy()
                  ? builder_.CreateLoad(rhs->getValue())
                  : rhs->getValue();
  if (!rval->getType()->isIntegerTy()) {
    throw error("Array's index must be integer, not " +
                    getNameString(rval->getType()),
                ast::attr(op).where, code_range_);
  }
  if (!lhs->getType()->isPointerTy()) {
    throw error("Cannot get element from non-pointer type " +
                    getNameString(lhs->getType()),
                ast::attr(op).where, code_range_);
  }

  auto lval = lhs->getValue();

  if (!lhs->getType()->getPointerElementType()->isArrayTy()) {
    lval = builder_.CreateLoad(lhs->getValue());

    if (!lval->getType()->getPointerElementType()->isArrayTy()) {
      throw error("Cannot get element from non-array type " +
                      getNameString(lhs->getType()),
                  ast::attr(op).where, code_range_);
    }
  }
  // Now lval's type is pointer to array

  std::vector<llvm::Value *> idxList = {builder_.getInt32(0), rval};
  auto ep =
      builder_.CreateInBoundsGEP(lval->getType()->getPointerElementType(), lval,
                                 llvm::ArrayRef<llvm::Value *>(idxList));
  if (ast::attr(op).lval)
    return new scoped_value(ep);
  else
    return new scoped_value(builder_.CreateLoad(ep));
}

scoped_value * translator::apply_op(ast::single_op<ast::load> const &op,
                              scoped_value * const value) {
  if (!value->getType()->isPointerTy())
    throw error("Cannot load from non-pointer type " +
                    getNameString(value->getType()),
                ast::attr(op).where, code_range_);
  return new scoped_value(builder_.CreateLoad(value->getValue()));
}

scoped_value *translator::apply_op(ast::single_op<ast::ret> const &op,
                                   scoped_value *const value) {
  return new scoped_value(builder_.CreateRet(value->getValue()));
}

scoped_value *translator::apply_op(ast::single_op<ast::lnot> const &,
                                   scoped_value *const value) {
  return new scoped_value(builder_.CreateXor(
      builder_.CreateICmpNE(value->getValue(),
                            llvm::Constant::getNullValue(builder_.getInt1Ty())),
      builder_.getInt32(1)));
}

scoped_value *translator::apply_op(ast::single_op<ast::inot> const &,
                                   scoped_value *const value) {
  return new scoped_value(
      builder_.CreateXor(value->getValue(), builder_.getInt32(1)));
}

scoped_value *translator::apply_op(ast::single_op<ast::inc> const &,
                                   scoped_value *const value) {
  return new scoped_value(
      builder_.CreateAdd(value->getValue(), builder_.getInt32(1)));
}

scoped_value *translator::apply_op(ast::single_op<ast::dec> const &,
                                   scoped_value *const value) {
  return new scoped_value(
      builder_.CreateSub(value->getValue(), builder_.getInt32(1)));
}

scoped_value *translator::apply_op(ast::ternary_op<ast::cond> const &,
                                   scoped_value *const first,
                                   scoped_value *const second,
                                   scoped_value *const third) {

  llvm::BasicBlock *thenbb = llvm::BasicBlock::Create(
      module_->getContext(), "", builder_.GetInsertBlock()->getParent());
  llvm::BasicBlock *elsebb = llvm::BasicBlock::Create(
      module_->getContext(), "", builder_.GetInsertBlock()->getParent());

  auto pb = builder_.GetInsertBlock();
  auto pp = builder_.GetInsertPoint();

  llvm::BasicBlock *mergebb = llvm::BasicBlock::Create(
      module_->getContext(), "", builder_.GetInsertBlock()->getParent());

  builder_.SetInsertPoint(thenbb);
  second->setBlock(thenbb);
  auto prevScope = currentScope_;
  currentScope_ = second;
  for (auto const &i : *(second->getInsts())) {
    boost::apply_visitor(*this, i);
  }
  builder_.CreateBr(mergebb);

  builder_.SetInsertPoint(elsebb);
  second->setBlock(elsebb);
  currentScope_ = third;
  for (auto const &i : *(third->getInsts())) {
    boost::apply_visitor(*this, i);
  }
  builder_.CreateBr(mergebb);

  currentScope_ = prevScope;
  builder_.SetInsertPoint(pb, pp);

  builder_.CreateCondBr(first->getValue(), thenbb, elsebb);

  builder_.SetInsertPoint(mergebb);

  return new scoped_value();
}

}; // namespace assembly
}; // namespace scopion
