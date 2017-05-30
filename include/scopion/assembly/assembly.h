#ifndef SCOPION_ASSEMBLY_H_
#define SCOPION_ASSEMBLY_H_

#include "scopion/ast/ast.h"
#include "scopion/parser/parser.h"

#include <iostream>

#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

namespace scopion {
namespace assembly {

class context {
public:
  llvm::LLVMContext llvmcontext;
};

class translator : public boost::static_visitor<llvm::Value *> {
  std::unique_ptr<llvm::Module> module_;
  llvm::IRBuilder<> builder_;

public:
  translator(std::unique_ptr<llvm::Module> &&module,
             llvm::IRBuilder<> const &builder);

  llvm::Value *operator()(parser::parsed const &expr);

  llvm::Value *operator()(ast::value value);

  llvm::Value *operator()(int value);
  llvm::Value *operator()(bool value);
  llvm::Value *operator()(std::string const &value);
  llvm::Value *operator()(ast::variable const &value);
  llvm::Value *operator()(ast::array const &value);
  llvm::Value *operator()(ast::function const &value);

  template <class Op> llvm::Value *operator()(ast::single_op<Op> const &op) {
    return apply_op(op, boost::apply_visitor(*this, op.value));
  }

  template <class Op> llvm::Value *operator()(ast::binary_op<Op> const &op) {
    llvm::Value *lhs = boost::apply_visitor(*this, op.lhs);
    llvm::Value *rhs = boost::apply_visitor(*this, op.rhs);

    return apply_op(op, lhs, rhs);
  }

  std::unique_ptr<llvm::Module> returnModule() { return std::move(module_); }

private:
  template <typename T> std::string getNameString(T *v) {
    std::string type_string;
    llvm::raw_string_ostream stream(type_string);
    v->print(stream);
    return stream.str();
  }
  llvm::Value *apply_op(ast::binary_op<ast::add> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::sub> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::mul> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::div> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::rem> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::shl> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::shr> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::iand> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::ior> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::ixor> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::land> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::lor> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::eeq> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::neq> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::gt> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::lt> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::gtq> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::ltq> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::assign> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::call> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::binary_op<ast::at> const &, llvm::Value *lhs,
                        llvm::Value *rhs);
  llvm::Value *apply_op(ast::single_op<ast::load> const &, llvm::Value *value);
  llvm::Value *apply_op(ast::single_op<ast::ret> const &, llvm::Value *value);
  llvm::Value *apply_op(ast::single_op<ast::lnot> const &, llvm::Value *value);
  llvm::Value *apply_op(ast::single_op<ast::inot> const &, llvm::Value *value);
  llvm::Value *apply_op(ast::single_op<ast::inc> const &, llvm::Value *value);
  llvm::Value *apply_op(ast::single_op<ast::dec> const &, llvm::Value *value);
};

class module {
  std::unique_ptr<llvm::Module> module_;

public:
  llvm::Value *val;

  static std::unique_ptr<module> create(parser::parsed const &tree,
                                        context &ctx,
                                        std::string const &name = "");

  std::string irgen();
  llvm::GenericValue run();

private:
  module(std::unique_ptr<llvm::Module> &&module, llvm::Value *val_)
      : module_(std::move(module)), val(val_) {}
};

}; // namespace assembly
}; // namespace scopion

#endif // SCOPION_ASSEMBLY_H_
