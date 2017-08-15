#include "scopion/ast/ast.hpp"

namespace scopion
{
namespace ast
{
class printer : boost::static_visitor<void>
{
  std::ostream& _s;

public:
  explicit printer(std::ostream& s) : _s(s) {}

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

  auto operator()(ast::integer val) const -> void { _s << ast::val(val); }

  auto operator()(ast::boolean val) const -> void { _s << std::boolalpha << ast::val(val); }

  auto operator()(ast::string const& val) const -> void { _s << "\"" << ast::val(val) << "\""; }

  auto operator()(variable const& val) const -> void
  {
    _s << ast::val(val);
    if (attr(val).to_call)
      _s << "{}";
    if (attr(val).lval)
      _s << "(lhs)";
  }

  auto operator()(identifier const& val) const -> void { _s << ast::val(val); }

  auto operator()(array const& val) const -> void
  {
    auto&& ary = ast::val(val);
    _s << "[ ";
    for (auto const& i : ary) {
      boost::apply_visitor(*this, i);
      _s << ", ";
    }
    _s << "]";
  }

  auto operator()(arglist const& val) const -> void
  {
    auto&& ary = ast::val(val);
    for (auto const& i : ary) {
      boost::apply_visitor(*this, i);
      _s << ", ";
    }
  }

  auto operator()(structure const& val) const -> void
  {
    auto&& ary = ast::val(val);
    _s << "{ ";
    for (auto const& i : ary) {
      (*this)(i.first);
      _s << ":";
      boost::apply_visitor(*this, i.second);
      _s << ", ";
    }
    _s << "}";
  }

  auto operator()(function const& val) const -> void
  {
    _s << "( ";
    for (auto const& arg : ast::val(val).first) {
      (*this)(arg);
      _s << ", ";
    }
    _s << "){ ";
    for (auto const& line : ast::val(val).second) {
      boost::apply_visitor(*this, line);
      _s << "; ";
    }
    _s << "} ";
  }

  auto operator()(scope const& val) const -> void
  {
    _s << "{ ";
    for (auto const& line : ast::val(val)) {
      boost::apply_visitor(*this, line);
      _s << "; ";
    }
    _s << "} ";
  }

  template <typename Op, size_t N>
  auto operator()(const op<Op, N>& o) const -> void
  {
    _s << "{ ";
    for (auto const& e : ast::val(o)) {
      boost::apply_visitor(*this, e);
      _s << " " << op_str<Op> << " ";
    }
    _s << " }";
    if (attr(o).lval)
      _s << "(lhs)";
  }

  template <typename T>
  auto operator()(const ternary_op<T>& o) const -> void
  {
    _s << "{ ";
    auto ops = op_str<T>;
    boost::apply_visitor(*this, ast::val(o)[0]);
    _s << " " << ops[0] << " ";
    boost::apply_visitor(*this, ast::val(o)[1]);
    _s << " " << ops[1] << " ";
    boost::apply_visitor(*this, ast::val(o)[2]);
    _s << " }";
    if (attr(o).lval)
      _s << "(lhs)";
  }
};  // class printer

std::ostream& operator<<(std::ostream& os, expr const& tree)
{
  boost::apply_visitor(printer(os), tree);
  return os;
}

};  // namespace ast
};  // namespace scopion
