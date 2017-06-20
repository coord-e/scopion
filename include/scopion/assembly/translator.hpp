#ifndef SCOPION_ASSEMBLY_TRANSLATOR_H_
#define SCOPION_ASSEMBLY_TRANSLATOR_H_

#include "scopion/assembly/scoped_value.hpp"
#include "scopion/ast/ast.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

namespace scopion {
namespace assembly {

class translator : public boost::static_visitor<scoped_value *> {
  std::shared_ptr<llvm::Module> module_;
  llvm::IRBuilder<> builder_;
  boost::iterator_range<std::string::const_iterator> const code_range_;
  scoped_value *currentScope_;

public:
  translator(std::shared_ptr<llvm::Module> &module,
             llvm::IRBuilder<> const &builder, std::string const &code);

  scoped_value *operator()(ast::value value);
  scoped_value *operator()(ast::operators value);

  scoped_value *operator()(ast::integer value);
  scoped_value *operator()(ast::boolean value);
  scoped_value *operator()(ast::string const &value);
  scoped_value *operator()(ast::variable const &value);
  scoped_value *operator()(ast::array const &value);
  scoped_value *operator()(ast::arglist const &value);
  scoped_value *operator()(ast::function const &value);
  scoped_value *operator()(ast::scope const &value);

  template <class Op> scoped_value *operator()(ast::single_op<Op> const &op) {
    return apply_op(op, boost::apply_visitor(*this, op.value));
  }

  template <class Op> scoped_value *operator()(ast::binary_op<Op> const &op) {
    scoped_value *lhs = boost::apply_visitor(*this, op.lhs);
    scoped_value *rhs = boost::apply_visitor(*this, op.rhs);

    return apply_op(op, lhs, rhs);
  }

  template <class Op> scoped_value *operator()(ast::ternary_op<Op> const &op) {
    scoped_value *first = boost::apply_visitor(*this, op.first);
    scoped_value *second = boost::apply_visitor(*this, op.second);
    scoped_value *third = boost::apply_visitor(*this, op.third);

    return apply_op(op, first, second, third);
  }

private:
  template <typename T> std::string getNameString(T *v) {
    std::string type_string;
    llvm::raw_string_ostream stream(type_string);
    v->print(stream);
    return stream.str();
  }
  scoped_value *apply_op(ast::binary_op<ast::add> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::sub> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::mul> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::div> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::rem> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::shl> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::shr> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::iand> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::ior> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::ixor> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::land> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::lor> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::eeq> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::neq> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::gt> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::lt> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::gtq> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::ltq> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::assign> const &,
                         scoped_value *const lhs, scoped_value *rhs);
  scoped_value *apply_op(ast::binary_op<ast::call> const &, scoped_value *lhs,
                         scoped_value *const rhs);
  scoped_value *apply_op(ast::binary_op<ast::at> const &,
                         scoped_value *const lhs, scoped_value *const rhs);
  scoped_value *apply_op(ast::single_op<ast::load> const &,
                         scoped_value *const value);
  scoped_value *apply_op(ast::single_op<ast::ret> const &,
                         scoped_value *const value);
  scoped_value *apply_op(ast::single_op<ast::lnot> const &,
                         scoped_value *const value);
  scoped_value *apply_op(ast::single_op<ast::inot> const &,
                         scoped_value *const value);
  scoped_value *apply_op(ast::single_op<ast::inc> const &,
                         scoped_value *const value);
  scoped_value *apply_op(ast::single_op<ast::dec> const &,
                         scoped_value *const value);
  scoped_value *apply_op(ast::ternary_op<ast::cond> const &,
                         scoped_value *const first, scoped_value *const second,
                         scoped_value *const third);
};

}; // namespace assembly
}; // namespace scopion

#endif
