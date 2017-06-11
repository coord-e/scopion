#include "scopion/assembly/scope.hpp"
#include "scopion/assembly/translator.hpp"

#include "scopion/error.hpp"

#include <boost/range/adaptor/indexed.hpp>

namespace scopion {
namespace assembly {

llvm::Value *translator::apply_op(ast::binary_op<ast::add> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateAdd(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::sub> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateSub(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::mul> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateMul(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::div> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateSDiv(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::rem> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateSRem(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::shl> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateShl(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::shr> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateLShr(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::iand> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateAnd(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::ior> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateOr(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::ixor> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateXor(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::land> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateAnd(
      builder_.CreateICmpNE(lhs,
                            llvm::Constant::getNullValue(builder_.getInt1Ty())),
      builder_.CreateICmpNE(
          rhs, llvm::Constant::getNullValue(builder_.getInt1Ty())));
}

llvm::Value *translator::apply_op(ast::binary_op<ast::lor> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateOr(
      builder_.CreateICmpNE(lhs,
                            llvm::Constant::getNullValue(builder_.getInt1Ty())),
      builder_.CreateICmpNE(
          rhs, llvm::Constant::getNullValue(builder_.getInt1Ty())));
}

llvm::Value *translator::apply_op(ast::binary_op<ast::eeq> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpEQ(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::neq> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpNE(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::gt> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpSGT(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::lt> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpSLT(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::gtq> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpSGE(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::ltq> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpSLE(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::assign> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  if (lhs == nullptr) { // first appear in the block (only variable)
    if (rhs->getType()->isVoidTy())
      throw error("Cannot assign the value of void type", ast::attr(op).where,
                  code_range_);
    auto &&lvar = boost::get<ast::variable>(boost::get<ast::value>(op.lhs));
    lhs = builder_.CreateAlloca(rhs->getType(), nullptr, ast::val(lvar));
    currentScope_->symbols[ast::val(lvar)] = lhs;
    builder_.CreateStore(rhs, lhs);
  } else {
    if (lhs->getType()->isPointerTy()) {
      if (lhs->getType()->getPointerElementType() == rhs->getType()) {
        builder_.CreateStore(rhs, lhs);
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
  }
  return builder_.CreateLoad(lhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::call> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
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

    args.push_back(v);
  }

  return builder_.CreateCall(lhs, llvm::ArrayRef<llvm::Value *>(args));
}

llvm::Value *translator::apply_op(ast::binary_op<ast::at> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  auto *rval = rhs->getType()->isPointerTy() ? builder_.CreateLoad(rhs) : rhs;
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

  auto *lval = lhs;

  if (!lhs->getType()->getPointerElementType()->isArrayTy()) {
    lval = builder_.CreateLoad(lhs);

    if (!lval->getType()->getPointerElementType()->isArrayTy()) {
      throw error("Cannot get element from non-array type " +
                      getNameString(lhs->getType()),
                  ast::attr(op).where, code_range_);
    }
  }
  // Now lval's type is pointer to array

  std::vector<llvm::Value *> idxList = {builder_.getInt32(0), rval};
  auto *ep =
      builder_.CreateInBoundsGEP(lval->getType()->getPointerElementType(), lval,
                                 llvm::ArrayRef<llvm::Value *>(idxList));
  if (ast::attr(op).lval)
    return ep;
  else
    return builder_.CreateLoad(ep);
}

llvm::Value *translator::apply_op(ast::single_op<ast::load> const &op,
                                  llvm::Value *value) {
  if (!value->getType()->isPointerTy())
    throw error("Cannot load from non-pointer type " +
                    getNameString(value->getType()),
                ast::attr(op).where, code_range_);
  return builder_.CreateLoad(value);
}

llvm::Value *translator::apply_op(ast::single_op<ast::ret> const &op,
                                  llvm::Value *value) {
  return builder_.CreateRet(value);
}

llvm::Value *translator::apply_op(ast::single_op<ast::lnot> const &,
                                  llvm::Value *value) {
  return builder_.CreateXor(
      builder_.CreateICmpNE(value,
                            llvm::Constant::getNullValue(builder_.getInt1Ty())),
      builder_.getInt32(1));
}

llvm::Value *translator::apply_op(ast::single_op<ast::inot> const &,
                                  llvm::Value *value) {
  return builder_.CreateXor(value, builder_.getInt32(1));
}

llvm::Value *translator::apply_op(ast::single_op<ast::inc> const &,
                                  llvm::Value *value) {
  return builder_.CreateAdd(value, builder_.getInt32(1));
}

llvm::Value *translator::apply_op(ast::single_op<ast::dec> const &,
                                  llvm::Value *value) {
  return builder_.CreateSub(value, builder_.getInt32(1));
}

}; // namespace assembly
}; // namespace scopion
