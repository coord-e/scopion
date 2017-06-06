#ifndef SCOPION_AST_H_
#define SCOPION_AST_H_

#include <boost/range/iterator_range.hpp>
#include <boost/variant.hpp>

#include <iostream>
#include <string>
#include <vector>

namespace scopion {

namespace ast {

struct add;
struct sub;
struct mul;
struct div;
struct rem;
struct shl;
struct shr;
struct iand;
struct ior;
struct ixor;
struct land;
struct lor;
struct eeq;
struct neq;
struct gt;
struct lt;
struct gtq;
struct ltq;
struct assign;
struct call;
struct at;

struct ret;
struct load;
struct lnot;
struct inot;
struct inc;
struct dec;

struct expr;

struct attribute {
  boost::iterator_range<std::string::const_iterator> where;
  bool lval = false;
  bool to_call = false;
};

template <typename T> class value_wrapper {
  T value;
  attribute attrib;

public:
  value_wrapper(T const &val) : value(val) {}
  value_wrapper() : value(T()) {}

  operator T() const { return value; }

  const T &get() const { return value; }
  attribute &attr() { return attrib; }
  const attribute &attr() const { return attrib; }
};

template <class T>
bool operator==(value_wrapper<T> const &lhs, value_wrapper<T> const &rhs);

template <class Op> struct single_op;
template <class Op> struct binary_op;
using integer = value_wrapper<int>;
using boolean = value_wrapper<bool>;
using string = value_wrapper<std::string>;
struct variable : string {
  using string::string;
};
using array = value_wrapper<std::vector<expr>>;
struct arglist : array {
  using array::array;
};
using function =
    value_wrapper<std::pair<std::vector<variable>, std::vector<expr>>>;

using value = boost::variant<
    integer, boolean, boost::recursive_wrapper<string>,
    boost::recursive_wrapper<variable>, boost::recursive_wrapper<array>,
    boost::recursive_wrapper<arglist>, boost::recursive_wrapper<function>>;

using expr_base =
    boost::variant<value, boost::recursive_wrapper<binary_op<add>>,
                   boost::recursive_wrapper<binary_op<sub>>,
                   boost::recursive_wrapper<binary_op<mul>>,
                   boost::recursive_wrapper<binary_op<div>>,
                   boost::recursive_wrapper<binary_op<rem>>,
                   boost::recursive_wrapper<binary_op<shl>>,
                   boost::recursive_wrapper<binary_op<shr>>,
                   boost::recursive_wrapper<binary_op<iand>>,
                   boost::recursive_wrapper<binary_op<ior>>,
                   boost::recursive_wrapper<binary_op<ixor>>,
                   boost::recursive_wrapper<binary_op<land>>,
                   boost::recursive_wrapper<binary_op<lor>>,
                   boost::recursive_wrapper<binary_op<eeq>>,
                   boost::recursive_wrapper<binary_op<neq>>,
                   boost::recursive_wrapper<binary_op<gt>>,
                   boost::recursive_wrapper<binary_op<lt>>,
                   boost::recursive_wrapper<binary_op<gtq>>,
                   boost::recursive_wrapper<binary_op<ltq>>,
                   boost::recursive_wrapper<binary_op<assign>>,
                   boost::recursive_wrapper<single_op<ret>>,
                   boost::recursive_wrapper<binary_op<call>>,
                   boost::recursive_wrapper<binary_op<at>>,
                   boost::recursive_wrapper<single_op<load>>,
                   boost::recursive_wrapper<single_op<lnot>>,
                   boost::recursive_wrapper<single_op<inot>>,
                   boost::recursive_wrapper<single_op<inc>>,
                   boost::recursive_wrapper<single_op<dec>>>;

struct expr : expr_base {
  using expr_base::expr_base;
  using expr_base::operator=;

  expr(expr_base const &ex) : expr_base(ex) {}
};
bool operator==(expr const &lhs, expr const &rhs);

template <class Op> struct single_op {
  expr value;
  attribute attrib;

  single_op(expr const &value_) : value(value_) {}

  attribute &attr() { return attrib; }
  const attribute &attr() const { return attrib; }
};
template <class Op>
bool operator==(single_op<Op> const &lhs, single_op<Op> const &rhs);

template <class Op> struct binary_op {
  expr lhs;
  expr rhs;
  attribute attrib;

  binary_op(expr const &lhs_, expr const &rhs_) : lhs(lhs_), rhs(rhs_) {}

  attribute &attr() { return attrib; }
  const attribute &attr() const { return attrib; }
};
template <class Op>
bool operator==(binary_op<Op> const &lhs, binary_op<Op> const &rhs);

std::ostream &operator<<(std::ostream &os, expr const &tree);

}; // namespace ast
}; // namespace scopion

#endif // SCOPION_AST_H_
