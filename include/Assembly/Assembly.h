#ifndef SCOPION_ASSEMBLY_H_
#define SCOPION_ASSEMBLY_H_

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

#include "AST/AST.h"
#include <iostream>

namespace scopion{

class assembly :
    public boost::static_visitor< llvm::Value* >
{
    llvm::LLVMContext context_;
    llvm::IRBuilder<> builder_;
    std::unique_ptr< llvm::Module > module_;
    std::map< std::string, llvm::Constant* > globals_;

public:
    assembly(std::string const& name);

    llvm::Value* operator()(ast::value value);

    template <class Op>
    llvm::Value* operator()(ast::binary_op< Op > const& op)
    {
        llvm::Value* lhs = boost::apply_visitor( *this, op.lhs );
        llvm::Value* rhs = boost::apply_visitor( *this, op.rhs );

        return apply_op( op, lhs, rhs );
    }

    void IRGen(std::vector< ast::expr > const& asts);
    std::string getIR();

private:
    llvm::Value* apply_op(ast::binary_op< ast::add > const&, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* apply_op(ast::binary_op< ast::sub > const&, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* apply_op(ast::binary_op< ast::mul > const&, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* apply_op(ast::binary_op< ast::div > const&, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* apply_op(ast::binary_op< ast::rem > const&, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* apply_op(ast::binary_op< ast::shl > const&, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* apply_op(ast::binary_op< ast::shr > const&, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* apply_op(ast::binary_op< ast::iand > const&, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* apply_op(ast::binary_op< ast::ior > const&, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* apply_op(ast::binary_op< ast::ixor > const&, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* apply_op(ast::binary_op< ast::land > const&, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* apply_op(ast::binary_op< ast::lor > const&, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* apply_op(ast::binary_op< ast::eeq > const&, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* apply_op(ast::binary_op< ast::neq > const&, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* apply_op(ast::binary_op< ast::gt > const&, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* apply_op(ast::binary_op< ast::lt > const&, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* apply_op(ast::binary_op< ast::gtq > const&, llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* apply_op(ast::binary_op< ast::ltq > const&, llvm::Value* lhs, llvm::Value* rhs);
    //llvm::Value* apply_op(ast::binary_op< ast::assign > const&, llvm::Value* lhs, llvm::Value* rhs);
};
};

#endif // SCOPION_ASSEMBLY_H_
