#include "scopion/ast/ast.h"

namespace scopion {
namespace ast {

template <typename Tc> struct compare_expr : boost::static_visitor<bool> {
public:
  Tc with;
  compare_expr(Tc const &_with) : with(_with) {}

  bool operator()(ast::expr_base exp) const {
    return boost::apply_visitor(*this, exp);
  }

  template <typename Op> bool operator()(ast::binary_op<Op> op) const {
    if (with.type() != typeid(op))
      return false;
    compare_expr<ast::expr_base> nr(boost::get<ast::binary_op<Op>>(with).rhs);
    compare_expr<ast::expr_base> nl(boost::get<ast::binary_op<Op>>(with).lhs);
    return boost::apply_visitor(nr, op.rhs) && boost::apply_visitor(nl, op.lhs);
  }

  bool operator()(ast::value value) const {
    if (with.type() != typeid(value))
      return false;
    compare_expr<ast::value> n(boost::get<ast::value>(with));
    return boost::apply_visitor(n, value);
  }

  template <typename T> bool operator()(T value) const {
    if (with.type() != typeid(value))
      return false;
    return value == boost::get<T>(with);
  }
};

bool operator==(expr const &lhs, expr const &rhs) {
  return boost::apply_visitor(compare_expr<expr>(lhs), rhs);
}
bool operator==(variable const &lhs, variable const &rhs) {
  return (lhs.name == rhs.name) && (lhs.lval == rhs.lval) &&
         (lhs.isFunc == rhs.isFunc);
}
bool operator==(array const &lhs, array const &rhs) {
  return (lhs.elements == rhs.elements);
}
bool operator==(function const &lhs, function const &rhs) {
  return (lhs.lines == rhs.lines);
}
template <class Op>
bool operator==(binary_op<Op> const &lhs, binary_op<Op> const &rhs) {
  return (lhs.rhs == rhs.rhs) && (lhs.lhs == rhs.lhs) && (lhs.lval == rhs.lval);
}

class printer : boost::static_visitor<void> {
  std::ostream &_s;

public:
  explicit printer(std::ostream &s) : _s(s) {}

  auto operator()(const value &v) const -> void {
    _s << "(";
    boost::apply_visitor(*this, v);
    _s << ")";
  }

  auto operator()(int value) const -> void { _s << value; }

  auto operator()(bool value) const -> void { _s << std::boolalpha << value; }

  auto operator()(std::string const &value) const -> void {
    _s << "\"" << value << "\"";
  }

  auto operator()(ast::variable const &value) const -> void {
    _s << value.name;
    if (value.isFunc)
      _s << "{}";
  }

  auto operator()(ast::array const &value) const -> void {
    auto &&ary = value.elements;
    _s << "[ ";
    for (auto const &i : ary) {
      boost::apply_visitor(*this, i);
      _s << ", ";
    }
    _s << "]";
  }

  auto operator()(ast::function const &value) const -> void {
    auto &&lines = value.lines;
    _s << "{ ";

    for (auto const &line : lines) {
      boost::apply_visitor(*this, line);
      _s << "; ";
    }
    _s << "} ";
  }

  template <typename T> auto operator()(const binary_op<T> &o) const -> void {
    _s << "{ ";
    boost::apply_visitor(*this, o.lhs);
    _s << " " << op_to_str(o) << " ";
    boost::apply_visitor(*this, o.rhs);
    _s << " }";
  }

private:
  std::string op_to_str(binary_op<add> const &) const { return "+"; }
  std::string op_to_str(binary_op<sub> const &) const { return "-"; }
  std::string op_to_str(binary_op<mul> const &) const { return "*"; }
  std::string op_to_str(binary_op<div> const &) const { return "/"; }
  std::string op_to_str(binary_op<rem> const &) const { return "%"; }
  std::string op_to_str(binary_op<shl> const &) const { return "<<"; }
  std::string op_to_str(binary_op<shr> const &) const { return ">>"; }
  std::string op_to_str(binary_op<iand> const &) const { return "&"; }
  std::string op_to_str(binary_op<ior> const &) const { return "|"; }
  std::string op_to_str(binary_op<ixor> const &) const { return "^"; }
  std::string op_to_str(binary_op<land> const &) const { return "&&"; }
  std::string op_to_str(binary_op<lor> const &) const { return "||"; }
  std::string op_to_str(binary_op<eeq> const &) const { return "=="; }
  std::string op_to_str(binary_op<neq> const &) const { return "!="; }
  std::string op_to_str(binary_op<gt> const &) const { return ">"; }
  std::string op_to_str(binary_op<lt> const &) const { return "<"; }
  std::string op_to_str(binary_op<gtq> const &) const { return ">="; }
  std::string op_to_str(binary_op<ltq> const &) const { return "<="; }
  std::string op_to_str(binary_op<assign> const &) const { return "="; }
  std::string op_to_str(binary_op<call> const &) const { return "()"; }
  std::string op_to_str(binary_op<at> const &) const { return "[]"; }
  std::string op_to_str(binary_op<load> const &) const { return "load"; }
  std::string op_to_str(binary_op<ret> const &) const { return "|>"; }
}; // class printer

std::ostream &operator<<(std::ostream &os, expr const &tree) {
  boost::apply_visitor(printer(os), tree);
  return os;
}

}; // namespace ast
}; // namespace scopion
