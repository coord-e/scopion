#ifndef SCOPION_AST_OPERATORS_H_
#define SCOPION_AST_OPERATORS_H_

#include "scopion/ast/attribute.h"
#include "scopion/ast/expr.h"

namespace scopion {
namespace ast {

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

}; // namespace ast
}; // namespace scopion

#endif
