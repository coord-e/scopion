#ifndef SCOPION_AST_VALUE_H_
#define SCOPION_AST_VALUE_H_

#include "scopion/ast/value_wrapper.hpp"

#include <boost/variant.hpp>

#include <map>
#include <string>
#include <vector>

namespace scopion
{
namespace ast
{
struct expr;

using integer = value_wrapper<int>;
using boolean = value_wrapper<bool>;
using string  = value_wrapper<std::string>;
struct variable : string {
  using string::string;
};
struct pre_variable : string {
  using string::string;
};
struct identifier : string {
  using string::string;
};
struct attribute_val : string {
  using string::string;
};
using array = value_wrapper<std::vector<expr>>;
struct arglist : array {
  using array::array;
};
using structure = value_wrapper<std::map<identifier, expr>>;
using function  = value_wrapper<std::pair<std::vector<identifier>, std::vector<expr>>>;

struct scope : array {
  using array::array;
};

using value = boost::variant<integer,
                             boolean,
                             boost::recursive_wrapper<string>,
                             boost::recursive_wrapper<variable>,
                             boost::recursive_wrapper<pre_variable>,
                             boost::recursive_wrapper<identifier>,
                             boost::recursive_wrapper<array>,
                             boost::recursive_wrapper<arglist>,
                             boost::recursive_wrapper<structure>,
                             boost::recursive_wrapper<function>,
                             boost::recursive_wrapper<scope>,
                             boost::recursive_wrapper<attribute_val>>;

};  // namespace ast
};  // namespace scopion

#endif
