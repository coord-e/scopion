#ifndef SCOPION_ASSEMBLY_TRANSLATOR_H_
#define SCOPION_ASSEMBLY_TRANSLATOR_H_

#include "scopion/assembly/scoped_value.hpp"
#include "scopion/ast/ast.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

namespace scopion {
namespace assembly {

using uniq_v_t = std::unique_ptr<scoped_value>;

class translator : public boost::static_visitor<uniq_v_t> {
  std::shared_ptr<llvm::Module> module_;
  llvm::IRBuilder<> builder_;
  boost::iterator_range<std::string::const_iterator> const code_range_;
  uniq_v_t currentScope_;

public:
  translator(std::shared_ptr<llvm::Module> &module,
             llvm::IRBuilder<> const &builder, std::string const &code);

  uniq_v_t operator()(ast::value value);
  uniq_v_t operator()(ast::operators value);

  uniq_v_t operator()(ast::integer value);
  uniq_v_t operator()(ast::boolean value);
  uniq_v_t operator()(ast::string const &value);
  uniq_v_t operator()(ast::variable const &value);
  uniq_v_t operator()(ast::array const &value);
  uniq_v_t operator()(ast::arglist const &value);
  uniq_v_t operator()(ast::function const &value);
  uniq_v_t operator()(ast::scope const &value);

  template <class Op> uniq_v_t operator()(ast::single_op<Op> const &op) {
    return apply_op(op, boost::apply_visitor(*this, op.value));
  }

  template <class Op> uniq_v_t operator()(ast::binary_op<Op> const &op) {
    uniq_v_t lhs = boost::apply_visitor(*this, op.lhs);
    uniq_v_t rhs = boost::apply_visitor(*this, op.rhs);

    return apply_op(op, std::move(lhs), std::move(rhs));
  }

private:
  template <typename T> std::string getNameString(T *v) {
    std::string type_string;
    llvm::raw_string_ostream stream(type_string);
    v->print(stream);
    return stream.str();
  }
  uniq_v_t apply_op(ast::binary_op<ast::add> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::sub> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::mul> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::div> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::rem> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::shl> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::shr> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::iand> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::ior> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::ixor> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::land> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::lor> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::eeq> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::neq> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::gt> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::lt> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::gtq> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::ltq> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::assign> const &, uniq_v_t const &&lhs,
                    uniq_v_t &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::call> const &, uniq_v_t &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::binary_op<ast::at> const &, uniq_v_t const &&lhs,
                    uniq_v_t const &&rhs);
  uniq_v_t apply_op(ast::single_op<ast::load> const &, uniq_v_t const &&value);
  uniq_v_t apply_op(ast::single_op<ast::ret> const &, uniq_v_t const &&value);
  uniq_v_t apply_op(ast::single_op<ast::lnot> const &, uniq_v_t const &&value);
  uniq_v_t apply_op(ast::single_op<ast::inot> const &, uniq_v_t const &&value);
  uniq_v_t apply_op(ast::single_op<ast::inc> const &, uniq_v_t const &&value);
  uniq_v_t apply_op(ast::single_op<ast::dec> const &, uniq_v_t const &&value);
};

}; // namespace assembly
}; // namespace scopion

#endif
