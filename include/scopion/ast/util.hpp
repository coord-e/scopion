/**
* @file util.hpp
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

#ifndef SCOPION_AST_UTIL_H_
#define SCOPION_AST_UTIL_H_

#include "scopion/ast/attribute.hpp"
#include "scopion/ast/expr.hpp"
#include "scopion/ast/value_wrapper.hpp"

#include "scopion/error.hpp"

#include <boost/variant.hpp>

#include <string>

namespace scopion
{
namespace ast
{
template <typename T>
T& val(value_wrapper<T>& w)
{
  return w.value;
}

template <typename T>
const T& val(value_wrapper<T> const& w)
{
  return w.value;
}

template <typename T>
attribute& attr(T& w)
{
  return w.attr;
}
template <typename T>
const attribute& attr(T const& w)
{
  return w.attr;
}

namespace visitors_
{
class setter_visitor : boost::static_visitor<expr>
{
  std::function<void(attribute&)> f_;

public:
  template <typename T>
  setter_visitor(T f) : f_(f)
  {
  }

  template <typename T>
  expr operator()(T val) const
  {
    f_(attr(val));
    return val;
  }

  expr operator()(expr val) const { return boost::apply_visitor(*this, val); }

  expr operator()(value val) const { return boost::apply_visitor(*this, val); }

  expr operator()(operators val) const { return boost::apply_visitor(*this, val); }
};

class setter_recursive_visitor : boost::static_visitor<expr>
{
  std::function<void(attribute&)> f_;

public:
  template <typename T>
  setter_recursive_visitor(T f) : f_(f)
  {
  }

  template <typename Op, size_t N>
  expr operator()(op<Op, N> val) const
  {
    f_(attr(val));
    std::for_each(ast::val(val).begin(), ast::val(val).end(),
                  [this](auto& x) { x = boost::apply_visitor(*this, x); });
    return val;
  }

  template <typename T>
  expr operator()(T val) const
  {
    f_(attr(val));
    return val;
  }

  expr operator()(expr val) const { return boost::apply_visitor(*this, val); }

  expr operator()(value val) const { return boost::apply_visitor(*this, val); }

  expr operator()(operators val) const { return boost::apply_visitor(*this, val); }
};

};  // namespace visitors_

template <typename T>
expr set_lval(T t, bool val)
{
  return visitors_::setter_visitor([val](attribute& attr) { attr.lval = val; })(t);
}

template <typename T>
expr set_to_call(T t, bool val)
{
  return visitors_::setter_visitor([val](attribute& attr) { attr.to_call = val; })(t);
}

template <typename T>
expr set_survey(T t, bool val)
{
  return visitors_::setter_recursive_visitor([val](attribute& attr) { attr.survey = val; })(t);
}

template <typename T>
expr set_attr(T t, std::string const& key, std::string const& val)
{
  return visitors_::setter_visitor([key, val](attribute& attr) { attr.attributes[key] = val; })(t);
}

template <typename T>
T set_where(T val, locationInfo range)
{
  attr(val).where = range;
  return val;
}

template <typename Dest, std::enable_if_t<std::is_convertible<Dest, value>::value>* = nullptr>
Dest& unpack(expr t)
{
  return boost::get<Dest>(boost::get<value>(t));
}

template <typename Dest, std::enable_if_t<std::is_convertible<Dest, operators>::value>* = nullptr>
Dest& unpack(expr t)
{
  return boost::get<Dest>(boost::get<operators>(t));
}

template <typename Dest>
bool isa(expr t)
{
  if (t.type() == typeid(value))
    return boost::get<value>(t).type() == typeid(Dest);
  else if (t.type() == typeid(operators))
    return boost::get<operators>(t).type() == typeid(Dest);
  else
    return false;
}

};  // namespace ast
};  // namespace scopion

#endif
