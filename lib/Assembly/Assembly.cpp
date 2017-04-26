#include "Assembly/Assembly.h"
#include <iostream>

namespace scopion{

assembly::assembly(std::string const& name) :
      boost::static_visitor< llvm::Value* >(),
      module_( new llvm::Module( name, context_ ) ),
      builder_( context_ )
{
  auto* main_func = llvm::Function::Create(
      llvm::FunctionType::get( builder_.getInt32Ty(), false ),
      llvm::Function::ExternalLinkage,
      "main",
      module_.get()
  );

  builder_.SetInsertPoint( llvm::BasicBlock::Create( context_, "entry", main_func ) );

  std::vector< llvm::Type* > args = {
      builder_.getInt8Ty()->getPointerTo()
  };

  globals_["printf"] = module_->getOrInsertFunction(
      "printf",
      llvm::FunctionType::get( builder_.getInt32Ty(), llvm::ArrayRef< llvm::Type* >( args ), true )
  );

}

llvm::Value* assembly::operator()(ast::value value)
{
  switch(value.which()){
    case 0://int
      return llvm::ConstantInt::get( builder_.getInt32Ty(), boost::get<int>(value) );
    case 1://bool
      return llvm::ConstantInt::get( builder_.getInt1Ty(), boost::get<bool>(value) );
    case 2://string
      return builder_.CreateGlobalStringPtr(boost::get<std::string>(value));
    default:
      assert( false );
  }
}

{

void assembly::IRGen(std::vector< ast::expr > const& asts)
{
  for( auto const& i : asts ) {
      std::vector< llvm::Value* > args = {
          builder_.CreateGlobalStringPtr( "%d\n" ), boost::apply_visitor( *this, i )
      };
      builder_.CreateCall( globals_["printf"], llvm::ArrayRef< llvm::Value* >( args ) );
  }

  builder_.CreateRet( llvm::ConstantInt::get( builder_.getInt32Ty(), 0 ) );
}

std::string assembly::getIR()
{
  std::string result;
  llvm::raw_string_ostream stream( result );

  module_->print( stream, nullptr );
  return result;
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
