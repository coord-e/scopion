#ifndef SCOPION_AST_EXPR_H_
#define SCOPION_AST_EXPR_H_

#include "scopion/ast/operators.hpp"
#include "scopion/ast/value.hpp"

#include <boost/variant.hpp>

#include <algorithm>
#include <array>
#include <initializer_list>
#include <iostream>
#include <type_traits>

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
  std::array<expr, N> exprs;

  op_base(std::initializer_list<expr> args) {
    assert(N == args.size() &&
           "Initialization with wrong number of expression");
    std::copy(args.begin(), args.end(), exprs.begin());
  }
  op_base(std::array<expr, N> const &args) : exprs(args) {}

  inline expr const &operator[](size_t idx) const { return exprs[idx]; }
};
template <class Op, size_t N>
bool operator==(op_base<Op, N> const &lhs, op_base<Op, N> const &rhs);

}; // namespace ast
}; // namespace scopion

std::ostream &operator<<(std::ostream &os, scopion::ast::expr const &tree);

#endif
