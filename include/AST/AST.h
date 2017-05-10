#ifndef SCOPION_AST_H_
#define SCOPION_AST_H_

#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_LIST_SIZE 50

#include <boost/variant.hpp>
#include <iostream>
#include <string>
#include <vector>

namespace scopion {

namespace ast {

struct add;
struct sub;
struct mul;
struct div;
struct rem;
struct shl;
struct shr;
struct iand;
struct ior;
struct ixor;
struct land;
struct lor;
struct eeq;
struct neq;
struct gt;
struct lt;
struct gtq;
struct ltq;
struct assign;
struct call;
struct at;

template <class Op> struct binary_op;
class variable;
class array;

using value = boost::variant<
    int, bool,
    boost::recursive_wrapper<std::string>, // TODO: figure out why
                                           // std::string needs
                                           // wrapper to work
    boost::recursive_wrapper<variable>, boost::recursive_wrapper<array>>;

class variable {
public:
  std::string name;
  bool rl;
  bool isFunc;
  variable(std::string const &n, bool rorl, bool isfunc)
      : name(n), rl(rorl), isFunc(isfunc) {}
};

using expr = boost::variant<value, boost::recursive_wrapper<binary_op<add>>,
                            boost::recursive_wrapper<binary_op<sub>>,
                            boost::recursive_wrapper<binary_op<mul>>,
                            boost::recursive_wrapper<binary_op<div>>,
                            boost::recursive_wrapper<binary_op<rem>>,
                            boost::recursive_wrapper<binary_op<shl>>,
                            boost::recursive_wrapper<binary_op<shr>>,
                            boost::recursive_wrapper<binary_op<iand>>,
                            boost::recursive_wrapper<binary_op<ior>>,
                            boost::recursive_wrapper<binary_op<ixor>>,
                            boost::recursive_wrapper<binary_op<land>>,
                            boost::recursive_wrapper<binary_op<lor>>,
                            boost::recursive_wrapper<binary_op<eeq>>,
                            boost::recursive_wrapper<binary_op<neq>>,
                            boost::recursive_wrapper<binary_op<gt>>,
                            boost::recursive_wrapper<binary_op<lt>>,
                            boost::recursive_wrapper<binary_op<gtq>>,
                            boost::recursive_wrapper<binary_op<ltq>>,
                            boost::recursive_wrapper<binary_op<assign>>,
                            boost::recursive_wrapper<binary_op<call>>,
                            boost::recursive_wrapper<binary_op<at>>>;

class array {
public:
  std::vector<expr> elements;
  array(std::vector<expr> const &elms_) : elements(elms_) {}
};

template <class Op> struct binary_op {
  expr lhs;
  expr rhs;

  binary_op(expr const &lhs_, expr const &rhs_) : lhs(lhs_), rhs(rhs_) {}
};

class printer : boost::static_visitor<void> {
  std::ostream &_s;

public:
  explicit printer(std::ostream &s) : _s(s) {}

  auto operator()(const value &v) const -> void {
    _s << "(";
    switch (v.which()) {
    case 0: // int
      std::cout << boost::get<int>(v);
      break;
    case 1: // bool
      std::cout << std::boolalpha << boost::get<bool>(v);
      break;
    case 2: // string
      std::cout << "\"" << boost::get<std::string>(v) << "\"";
      break;
    case 3: // variable
    {
      auto &&var = boost::get<ast::variable>(v);
      std::cout << var.name;
      if (var.isFunc)
        std::cout << "{}";
      break;
    }
    case 4: // array
    {
      auto &&ary = boost::get<ast::array>(v).elements;
      std::cout << "[ ";
      for (auto const &i : ary) {
        boost::apply_visitor(printer(_s), i);
        std::cout << ", ";
      }
      std::cout << "]";
    }
    }
    _s << ")";
  }

  template <typename T> auto operator()(const binary_op<T> &o) const -> void {
    _s << "{ ";
    boost::apply_visitor(printer(_s), o.lhs);
    _s << " " << op_to_str(o) << " ";
    boost::apply_visitor(printer(_s), o.rhs);
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
}; // class printer

}; // namespace ast
}; // namespace scopion

#endif // SCOPION_AST_H_
