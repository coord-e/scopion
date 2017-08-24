/**
* @file compare_expr.cpp
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

#include "scopion/ast/ast.hpp"

namespace scopion
{
namespace ast
{
template <typename Tc>
struct compare_expr : boost::static_visitor<bool> {
public:
  Tc with;
  compare_expr(Tc const& _with) : with(_with) {}

  bool operator()(ast::expr_base exp) const { return boost::apply_visitor(*this, exp); }

  template <typename Op, size_t N>
  bool operator()(ast::op<Op, N> o) const
  {
    if (with.type() != typeid(o))
      return false;
    for (size_t i = 0; i < N; i++) {
      if (!boost::apply_visitor(
              compare_expr<ast::expr_base>(ast::val(boost::get<ast::op<Op, N>>(with))[i]),
              ast::val(o)[i]))
        return false;
    }
    return true;
  }

  bool operator()(ast::value value) const
  {
    if (with.type() != typeid(value))
      return false;
    compare_expr<ast::value> n(boost::get<ast::value>(with));
    return boost::apply_visitor(n, value);
  }

  bool operator()(ast::operators value) const
  {
    if (with.type() != typeid(value))
      return false;
    compare_expr<ast::operators> n(boost::get<ast::operators>(with));
    return boost::apply_visitor(n, value);
  }

  template <typename T>
  bool operator()(T value) const
  {
    if (with.type() != typeid(value))
      return false;
    return value == boost::get<T>(with);
  }
};

bool operator==(attribute const& lhs, attribute const& rhs)
{
  return (lhs.lval == rhs.lval) && (lhs.to_call == rhs.to_call) &&
         (lhs.attributes == rhs.attributes);
}

template <class T>
bool operator==(value_wrapper<T> const& lhs, value_wrapper<T> const& rhs)
{
  return (ast::val(lhs) == ast::val(rhs)) && (ast::attr(rhs) == ast::attr(lhs));
}

bool operator==(expr const& lhs, expr const& rhs)
{
  return boost::apply_visitor(compare_expr<expr>(lhs), rhs);
}

template <typename Op, size_t N>
bool operator==(op_base<Op, N> const& lhs, op_base<Op, N> const& rhs)
{
  return lhs.exprs_ == rhs.exprs_;
}

}  // namespace ast
}  // namespace scopion
