#ifndef SCOPION_AST_VALUE_H_
#define SCOPION_AST_VALUE_H_

#include "scopion/ast/expr.hpp"
#include "scopion/ast/value_wrapper.hpp"

#include <boost/variant.hpp>

#include <string>
#include <vector>

namespace scopion {
namespace ast {

struct expr;

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

}; // namespace ast
}; // namespace scopion

#endif
