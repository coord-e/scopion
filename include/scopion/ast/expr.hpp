#ifndef SCOPION_AST_EXPR_H_
#define SCOPION_AST_EXPR_H_

#include "scopion/ast/operators.hpp"
#include "scopion/ast/value.hpp"

#include <boost/variant.hpp>

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

template <class Op> struct single_op {
  expr value;
  attribute attr;

  single_op(expr const &value_) : value(value_) {}
};
template <class Op>
bool operator==(single_op<Op> const &lhs, single_op<Op> const &rhs);

template <class Op> struct binary_op {
  expr lhs;
  expr rhs;
  attribute attr;

  binary_op(expr const &lhs_, expr const &rhs_) : lhs(lhs_), rhs(rhs_) {}
};
template <class Op>
bool operator==(binary_op<Op> const &lhs, binary_op<Op> const &rhs);

template <class Op> struct ternary_op {
  expr first;
  expr second;
  expr third;
  attribute attr;

  ternary_op(expr const &first_, expr const &second_, expr const &third_)
      : first(first_), second(second_), third(third_) {}
};
template <class Op>
bool operator==(ternary_op<Op> const &lhs, ternary_op<Op> const &rhs);

}; // namespace ast
}; // namespace scopion

std::ostream &operator<<(std::ostream &os, scopion::ast::expr const &tree);

#endif
