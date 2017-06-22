#ifndef SCOPION_AST_EXPR_H_
#define SCOPION_AST_EXPR_H_

#include "scopion/ast/operators.hpp"
#include "scopion/ast/value.hpp"

#include <boost/variant.hpp>

#include <array>
#include <iostream>

namespace scopion {
namespace ast {

using expr_base = boost::variant<value, operators>;

struct expr : expr_base {
  using expr_base::expr_base;
  using expr_base::operator=;

  expr(expr_base const &ex) : expr_base(ex) {}
};
bool operator==(expr const &lhs, expr const &rhs);

template <class Op, size_t N> struct op_base {
  std::vector<expr> const &exprs;

  /*template <typename... Args,
            std::enable_if_t<sizeof...(Args) == N> * = nullptr>
  op_base(Args... args) : exprs({args...}) {}*/
  op_base(std::vector<expr> const &args) : exprs(args) {}
};
template <class Op, size_t N>
bool operator==(op_base<Op, N> const &lhs, op_base<Op, N> const &rhs);

}; // namespace ast
}; // namespace scopion

std::ostream &operator<<(std::ostream &os, scopion::ast::expr const &tree);

#endif
