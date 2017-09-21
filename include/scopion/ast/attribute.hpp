/**
* @file attribute.hpp
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

#ifndef SCOPION_AST_ATTRIBUTE_H_
#define SCOPION_AST_ATTRIBUTE_H_

#include "scopion/error.hpp"

#include <boost/range/iterator_range.hpp>

#include <string>
#include <unordered_map>

namespace scopion
{
namespace ast
{
struct attribute {
  locationInfo where;
  std::unordered_map<std::string, std::string> attributes;
  bool lval    = false;
  bool to_call = false;
  bool survey  = false;
};
bool operator==(attribute const& lhs, attribute const& rhs);

};  // namespace ast
};  // namespace scopion

#endif
