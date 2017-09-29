/**
* @file printer.hpp
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
#ifndef SCOPION_AST_PRINTER_H_
#define SCOPION_AST_PRINTER_H_

#include "scopion/ast/util.hpp"

namespace scopion
{
namespace ast
{
template <class Char, class Traits>
class printer : boost::static_visitor<void>
{
  std::basic_ostream<Char, Traits>& _s;

public:
  explicit printer(std::basic_ostream<Char, Traits>& s) : _s(s) {}

  auto operator()(const value& v) const -> void
  {
    _s << "(";
    boost::apply_visitor(*this, v);
    _s << ")";
  }

  auto operator()(const operators& v) const -> void
  {
    _s << "(";
    boost::apply_visitor(*this, v);
    _s << ")";
  }

  auto operator()(integer v) const -> void { _s << val(v); }

  auto operator()(decimal v) const -> void { _s << val(v); }

  auto operator()(boolean v) const -> void { _s << std::boolalpha << val(v); }

  auto operator()(string const& v) const -> void { _s << "\"" << val(v) << "\""; }

  auto operator()(variable const& v) const -> void
  {
    _s << val(v);
    if (attr(v).to_call)
      _s << "{}";
    if (attr(v).lval)
      _s << "(lhs)";
  }

  auto operator()(identifier const& v) const -> void { _s << val(v); }
  auto operator()(struct_key const& v) const -> void { _s << val(v); }

  auto operator()(array const& v) const -> void
  {
    auto&& ary = val(v);
    _s << "[ ";
    for (auto const& i : ary) {
      boost::apply_visitor(*this, i);
      _s << ", ";
    }
    _s << "]";
  }

  auto operator()(arglist const& v) const -> void
  {
    auto&& ary = val(v);
    for (auto const& i : ary) {
      boost::apply_visitor(*this, i);
      _s << ", ";
    }
  }

  auto operator()(structure const& v) const -> void
  {
    auto&& ary = val(v);
    _s << "{ ";
    for (auto const& i : ary) {
      (*this)(i.first);
      _s << ":";
      boost::apply_visitor(*this, i.second);
      _s << ", ";
    }
    _s << "}";
  }

  auto operator()(function const& v) const -> void
  {
    _s << "( ";
    for (auto const& arg : val(v).first) {
      (*this)(arg);
      _s << ", ";
    }
    _s << "){ ";
    for (auto const& line : val(v).second) {
      boost::apply_visitor(*this, line);
      _s << "; ";
    }
    _s << "} ";
  }

  auto operator()(scope const& v) const -> void
  {
    _s << "{ ";
    for (auto const& line : val(v)) {
      boost::apply_visitor(*this, line);
      _s << "; ";
    }
    _s << "} ";
  }

  template <typename Op, size_t N>
  auto operator()(const op<Op, N>& o) const -> void
  {
    _s << "{ ";
    for (auto const& e : val(o)) {
      boost::apply_visitor(*this, e);
      _s << " " << Op::str << " ";
    }
    _s << " }";
    if (attr(o).lval)
      _s << "(lhs)";
  }

  template <typename T>
  auto operator()(const ternary_op<T>& o) const -> void
  {
    _s << "{ ";
    auto ops = T::str;
    boost::apply_visitor(*this, val(o)[0]);
    _s << " " << ops[0] << " ";
    boost::apply_visitor(*this, val(o)[1]);
    _s << " " << ops[1] << " ";
    boost::apply_visitor(*this, val(o)[2]);
    _s << " }";
    if (attr(o).lval)
      _s << "(lhs)";
  }

};  // class printer

template <class Char, class Traits>
std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& os, expr const& tree)
{
  boost::apply_visitor(printer<Char, Traits>(os), tree);
  return os;
}

};  // namespace ast
};  // namespace scopion

#endif
