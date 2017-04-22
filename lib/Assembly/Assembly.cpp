#include "Assembly/Assembly.h"
#include <iostream>

namespace scopion{

assembly::assembly(llvm::IRBuilder<>& builder) :
      boost::static_visitor< llvm::Value* >(),
      builder_( builder )
{ }

llvm::Value* assembly::operator()(int value)
{
    return llvm::ConstantInt::get( builder_.getInt32Ty(), value );
}

llvm::Value* assembly::operator()(bool value)
{
    return llvm::ConstantInt::get( builder_.getInt1Ty(), value );
}

llvm::Value* assembly::operator()(std::string value)
{
    return builder_.CreateGlobalStringPtr(value);
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::add > const& op, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateAdd( lhs, rhs );
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::sub > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateSub( lhs, rhs );
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::mul > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateMul( lhs, rhs );
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::div > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateSDiv( lhs, rhs );
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::rem > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateSRem( lhs, rhs );
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::shl > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateShl( lhs, rhs );
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::shr > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateLShr( lhs, rhs );
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::iand > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateAnd( lhs, rhs );
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::ior > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateOr( lhs, rhs );
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::ixor > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateXor( lhs, rhs );
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::land > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateAnd(builder_.CreateICmpNE( lhs, llvm::Constant::getNullValue(builder_.getInt1Ty())),
                              builder_.CreateICmpNE( rhs, llvm::Constant::getNullValue(builder_.getInt1Ty())));
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::lor > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateOr(builder_.CreateICmpNE( lhs, llvm::Constant::getNullValue(builder_.getInt1Ty())),
                             builder_.CreateICmpNE( rhs, llvm::Constant::getNullValue(builder_.getInt1Ty())));
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::eeq > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateICmpEQ( lhs, rhs );
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::neq > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateICmpNE( lhs, rhs );
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::gt > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateICmpSGT( lhs, rhs );
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::lt > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateICmpSLT( lhs, rhs );
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::gtq > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateICmpSGE( lhs, rhs );
}

llvm::Value* assembly::apply_op(ast::binary_op< ast::ltq > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    return builder_.CreateICmpSLE( lhs, rhs );
}

/*llvm::Value* assembly::apply_op(ast::binary_op< ast::assign > const&, llvm::Value* lhs, llvm::Value* rhs)
{
    builder_.CreateAlloca(builder_.getInt32Ty());
    return rhs;
}*/

}; //namespace scopion
