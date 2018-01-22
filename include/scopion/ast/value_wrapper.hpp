/**
* @file value_wrapper.hpp
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

#ifndef SCOPION_AST_VALUE_WRAPPER_H_
#define SCOPION_AST_VALUE_WRAPPER_H_

#include "scopion/ast/attribute.hpp"

#include <type_traits>

namespace scopion
{
namespace ast
{
template <typename T>
class value_wrapper
{
public:
  T value;
  attribute attr;

  value_wrapper(T const& val) : value(val) {}
  template <typename U, std::enable_if_t<std::is_constructible<T, U>::value>* = nullptr>
  value_wrapper(U const& val) : value(T(val))
  {
  }
  value_wrapper() : value(T()) {}
};
template <class T>
bool operator==(value_wrapper<T> const& lhs, value_wrapper<T> const& rhs);
template <class T>
bool operator<(value_wrapper<T> const& lhs, value_wrapper<T> const& rhs)
{
  return lhs.value < rhs.value;
}

}  // namespace ast
}  // namespace scopion

#endif
