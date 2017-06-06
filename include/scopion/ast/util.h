#ifndef SCOPION_AST_UTIL_H_
#define SCOPION_AST_UTIL_H_

#include "scopion/ast/attribute.h"
#include "scopion/ast/expr.h"
#include "scopion/ast/value_wrapper.h"

#include <boost/variant.hpp>

namespace scopion {
namespace ast {

struct set_lval_visitor : boost::static_visitor<expr> {
  const bool v;

  set_lval_visitor(bool v_) : v(v_) {}

  template <typename T> expr operator()(T val) const {
    attr(val).lval = v;
    return val;
  }

  ast::expr operator()(expr val) const {
    return boost::apply_visitor(*this, val);
  }

  ast::expr operator()(value val) const {
    return boost::apply_visitor(*this, val);
  }
};

struct set_to_call_visitor : boost::static_visitor<expr> {
  const bool v;

  set_to_call_visitor(bool v_) : v(v_) {}

  template <typename T> expr operator()(T val) const {
    attr(val).to_call = v;
    return val;
  }

  ast::expr operator()(expr val) const {
    return boost::apply_visitor(*this, val);
  }

  ast::expr operator()(value val) const {
    return boost::apply_visitor(*this, val);
  }
};

template <typename T> T &val(value_wrapper<T> &w) { return w.value; }

template <typename T> const T &val(value_wrapper<T> const &w) {
  return w.value;
}

template <typename T> attribute &attr(T &w) { return w.attr; }
template <typename T> const attribute &attr(T const &w) { return w.attr; }

template <typename T> expr set_lval(T t, bool val) {
  return set_lval_visitor(val)(t);
}

template <typename T> expr set_to_call(T t, bool val) {
  return set_to_call_visitor(val)(t);
}

template <typename T, typename RangeT> T set_where(T val, RangeT range) {
  attr(val).where = range;
  return val;
}

}; // namespace ast
}; // namespace scopion

#endif
