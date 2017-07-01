#ifndef SCOPION_AST_UTIL_H_
#define SCOPION_AST_UTIL_H_

#include "scopion/ast/attribute.hpp"
#include "scopion/ast/expr.hpp"
#include "scopion/ast/value_wrapper.hpp"

#include <boost/variant.hpp>

#include <string>

namespace scopion {
namespace ast {

template <typename T> T &val(value_wrapper<T> &w) { return w.value; }

template <typename T> const T &val(value_wrapper<T> const &w) {
  return w.value;
}

template <typename T> attribute &attr(T &w) { return w.attr; }
template <typename T> const attribute &attr(T const &w) { return w.attr; }

namespace visitors_ {

struct setter_visitor : boost::static_visitor<expr> {

  virtual void process(attribute &val) const {}

  template <typename T> expr operator()(T val) const {
    process(attr(val));
    return val;
  }

  expr operator()(expr val) const { return boost::apply_visitor(*this, val); }

  expr operator()(value val) const { return boost::apply_visitor(*this, val); }

  expr operator()(operators val) const {
    return boost::apply_visitor(*this, val);
  }
};

struct setter_recursive_visitor : setter_visitor {

  using setter_visitor::operator();

  template <typename Op> expr operator()(single_op<Op> val) const {
    process(attr(val));
    val.value = boost::apply_visitor(*this, val.value);
    return val;
  }

  template <typename Op> expr operator()(binary_op<Op> val) const {
    process(attr(val));
    val.lhs = boost::apply_visitor(*this, val.lhs);
    val.rhs = boost::apply_visitor(*this, val.rhs);
    return val;
  }

  template <typename Op> expr operator()(ternary_op<Op> val) const {
    process(attr(val));
    val.first = boost::apply_visitor(*this, val.first);
    val.second = boost::apply_visitor(*this, val.second);
    val.third = boost::apply_visitor(*this, val.third);
    return val;
  }
};

struct set_lval_visitor : setter_recursive_visitor {
  using setter_recursive_visitor::operator();

  const bool v;

  set_lval_visitor(bool v_) : v(v_) {}

  virtual void process(attribute &val) const { val.lval = v; }
};

struct set_to_call_visitor : setter_recursive_visitor {
  using setter_recursive_visitor::operator();

  const bool v;

  set_to_call_visitor(bool v_) : v(v_) {}

  virtual void process(attribute &val) const { val.to_call = v; }
};

struct set_survey_visitor : setter_recursive_visitor {
  using setter_recursive_visitor::operator();

  const bool v;

  set_survey_visitor(bool v_) : v(v_) {}

  virtual void process(attribute &val) const { val.survey = v; }
};

struct set_attr_visitor : setter_visitor {
  using setter_visitor::operator();

  std::string const &key;
  std::string const &val;

  set_attr_visitor(std::string const &key_, std::string const &val_)
      : key(key_), val(val_) {}

  virtual void process(attribute &v) const { v.attributes[key] = val; }
};

}; // namespace visitors_

template <typename T> expr set_lval(T t, bool val) {
  return visitors_::set_lval_visitor(val)(t);
}

template <typename T> expr set_to_call(T t, bool val) {
  return visitors_::set_to_call_visitor(val)(t);
}

template <typename T> expr set_survey(T t, bool val) {
  return visitors_::set_survey_visitor(val)(t);
}

template <typename T>
expr set_attr(T t, std::string const &key, std::string const &val) {
  return visitors_::set_attr_visitor(key, val)(t);
}

template <typename T, typename RangeT> T set_where(T val, RangeT range) {
  attr(val).where = range;
  return val;
}

template <typename Dest,
          std::enable_if_t<std::is_convertible<Dest, value>::value> * = nullptr>
Dest unpack(expr t) {
  return boost::get<Dest>(boost::get<value>(t));
}

template <
    typename Dest,
    std::enable_if_t<std::is_convertible<Dest, operators>::value> * = nullptr>
Dest unpack(expr t) {
  return boost::get<Dest>(boost::get<operators>(t));
}

}; // namespace ast
}; // namespace scopion

#endif
