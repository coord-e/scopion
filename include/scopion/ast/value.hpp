/**
* @file value.hpp
*
* (c) copyright 2017 coord.e
*
* This file is part of scopion.
*
* scopion is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* scopion is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with scopion.  If not, see <http://www.gnu.org/licenses/>.
*/

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
using decimal = value_wrapper<double>;
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
struct struct_key : string {
  using string::string;
};
struct attribute_val : string {
  using string::string;
};
using array = value_wrapper<std::vector<expr>>;
struct arglist : array {
  using array::array;
};
using structure = value_wrapper<std::map<struct_key, expr>>;
using function  = value_wrapper<std::pair<std::vector<identifier>, std::vector<expr>>>;

struct scope : array {
  using array::array;
};

using value = boost::variant<integer,
                             decimal,
                             boolean,
                             boost::recursive_wrapper<string>,
                             boost::recursive_wrapper<variable>,
                             boost::recursive_wrapper<pre_variable>,
                             boost::recursive_wrapper<identifier>,
                             boost::recursive_wrapper<struct_key>,
                             boost::recursive_wrapper<array>,
                             boost::recursive_wrapper<arglist>,
                             boost::recursive_wrapper<structure>,
                             boost::recursive_wrapper<function>,
                             boost::recursive_wrapper<scope>,
                             boost::recursive_wrapper<attribute_val>>;

};  // namespace ast
};  // namespace scopion

#endif
