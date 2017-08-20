#ifndef SCOPION_AST_UTIL_H_
#define SCOPION_AST_UTIL_H_

#include "scopion/ast/attribute.hpp"
#include "scopion/ast/expr.hpp"
#include "scopion/ast/value_wrapper.hpp"

#include <boost/variant.hpp>

#include <string>

namespace scopion
{
namespace ast
{
template <typename Op>
static constexpr auto op_str = "";

template <>
static constexpr auto op_str<add> = "+";
template <>
static constexpr auto op_str<sub> = "-";
template <>
static constexpr auto op_str<mul> = "*";
template <>
static constexpr auto op_str<div> = "/";
template <>
static constexpr auto op_str<rem> = "%";
template <>
static constexpr auto op_str<shl> = "<<";
template <>
static constexpr auto op_str<shr> = ">>";
template <>
static constexpr auto op_str<iand> = "&";
template <>
static constexpr auto op_str<ior> = "|";
template <>
static constexpr auto op_str<ixor> = "^";
template <>
static constexpr auto op_str<land> = "&&";
template <>
static constexpr auto op_str<lor> = "||";
template <>
static constexpr auto op_str<eeq> = "==";
template <>
static constexpr auto op_str<neq> = "!=";
template <>
static constexpr auto op_str<gt> = ">";
template <>
static constexpr auto op_str<lt> = "<";
template <>
static constexpr auto op_str<gtq> = ">=";
template <>
static constexpr auto op_str<ltq> = "<=";
template <>
static constexpr auto op_str<assign> = "=";
template <>
static constexpr auto op_str<call> = "()";
template <>
static constexpr auto op_str<at> = "[]";
template <>
static constexpr auto op_str<dot> = ".";
template <>
static constexpr auto op_str<odot> = ".:";
template <>
static constexpr auto op_str<adot> = ".=";
template <>
static constexpr auto op_str<ret> = "|>";
template <>
static constexpr auto op_str<lnot> = "!";
template <>
static constexpr auto op_str<inot> = "~";
template <>
static constexpr auto op_str<inc> = "++";
template <>
static constexpr auto op_str<dec> = "--";
template <>
static constexpr auto op_str<cond> = "?:";

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
protected:
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

struct setter_recursive_visitor : setter_visitor {
  using setter_visitor::operator();
  using setter_visitor::setter_visitor;

  template <typename Op, size_t N>
  expr operator()(op<Op, N> val) const
  {
    f_(attr(val));
    std::for_each(val.begin(), val.end(), [this](auto& x) { x = boost::apply_visitor(*this, x); });
    return val;
  }
};

};  // namespace visitors_

template <typename T>
expr set_lval(T t, bool val)
{
  return visitors_::setter_recursive_visitor([val](attribute& attr) { attr.lval = val; })(t);
}

template <typename T>
expr set_to_call(T t, bool val)
{
  return visitors_::setter_recursive_visitor([val](attribute& attr) { attr.to_call = val; })(t);
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

template <typename T, typename RangeT>
T set_where(T val, RangeT range)
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
