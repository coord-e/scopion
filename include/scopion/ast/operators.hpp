#ifndef SCOPION_AST_OPERATORS_H_
#define SCOPION_AST_OPERATORS_H_

#include "scopion/ast/attribute.hpp"
#include "scopion/ast/expr.hpp"
#include "scopion/ast/value_wrapper.hpp"

#include <boost/variant.hpp>

namespace scopion
{
namespace ast
{
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
struct cond;
struct dot;
struct at;

struct ret;
struct load;
struct lnot;
struct inot;
struct inc;
struct dec;

template <class Op, size_t N>
class op_base;
template <class Op, size_t N>
using op = value_wrapper<op_base<Op, N>>;

template <class Op>
using single_op = op<Op, 1>;
template <class Op>
using binary_op = op<Op, 2>;
template <class Op>
using ternary_op = op<Op, 3>;

using operators = boost::variant<boost::recursive_wrapper<binary_op<add>>,
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
                                 boost::recursive_wrapper<binary_op<dot>>,
                                 boost::recursive_wrapper<single_op<load>>,
                                 boost::recursive_wrapper<single_op<lnot>>,
                                 boost::recursive_wrapper<single_op<inot>>,
                                 boost::recursive_wrapper<single_op<inc>>,
                                 boost::recursive_wrapper<single_op<dec>>,
                                 boost::recursive_wrapper<ternary_op<cond>>>;
};  // namespace ast
};  // namespace scopion

#endif
