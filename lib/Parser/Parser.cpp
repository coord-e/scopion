#include "Parser/Parser.h"

#include <boost/spirit/home/x3.hpp>

#include <boost/algorithm/string.hpp>

#include <array>

#include "AST/AST.h"

namespace scopion {
namespace parser {

namespace x3 = boost::spirit::x3;

namespace grammar {

namespace detail {

decltype(auto) assign() {
  return [](auto &&ctx) { x3::_val(ctx) = x3::_attr(ctx); };
}

decltype(auto) assign_str() {
  return [](auto &&ctx) {
    auto &&v = x3::_attr(ctx);
    x3::_val(ctx) = std::string(v.begin(), v.end());
  };
}

template <typename T> decltype(auto) assign_as() {
  return [](auto &&ctx) { x3::_val(ctx) = T(x3::_attr(ctx)); };
}

decltype(auto) assign_var(bool rl = true) {
  return [rl](auto &&ctx) {
    auto &&v = x3::_attr(ctx);
    x3::_val(ctx) = ast::variable(std::string(v.begin(), v.end()), rl, false);
  };
}

template <typename Op> decltype(auto) assign_binop() {
  return [](auto &&ctx) {
    x3::_val(ctx) = ast::binary_op<Op>(x3::_val(ctx), x3::_attr(ctx));
  };
}

template <> decltype(auto) assign_binop<ast::assign>() {
  return [](auto &&ctx) {
    if (x3::_val(ctx).type() == typeid(ast::value)) {
      auto &&v = boost::get<ast::value>(x3::_val(ctx));
      if (v.type() == typeid(ast::variable)) {
        auto &&n = boost::get<ast::variable>(v).name;
        x3::_val(ctx) = ast::binary_op<ast::assign>(
            ast::variable(n, false, false), x3::_attr(ctx));
        return;
      }
    }
    x3::_val(ctx) = ast::binary_op<ast::assign>(x3::_val(ctx), x3::_attr(ctx));
  };
}

template <> decltype(auto) assign_binop<ast::call>() {
  return [](auto &&ctx) {
    if (x3::_val(ctx).type() != typeid(ast::value)) {
      x3::_val(ctx) = ast::binary_op<ast::call>(x3::_val(ctx), x3::_attr(ctx));
      return;
    }
    auto &&v = boost::get<ast::value>(x3::_val(ctx));
    if (v.type() == typeid(ast::variable)) {
      auto &&n = boost::get<ast::variable>(v).name;
      x3::_val(ctx) = ast::binary_op<ast::call>(ast::variable(n, false, true),
                                                x3::_attr(ctx));
    } else if (v.type() == typeid(ast::function)) {
      x3::_val(ctx) = ast::binary_op<ast::call>(x3::_val(ctx), x3::_attr(ctx));
    } else {
      throw std::runtime_error("lvalue have to be a variable or a function");
    }
  };
}

template <typename Op> decltype(auto) assign_sinop(ast::expr rv) {
  return [rv](auto &&ctx) {
    x3::_val(ctx) = ast::binary_op<Op>(x3::_attr(ctx), rv);
  };
}

} // namespace detail

struct primary;
struct call_expr;
struct pre_sinop_expr;
struct post_sinop_expr;
struct mul_expr;
struct add_expr;
struct shift_expr;
struct cmp_expr;
struct iand_expr;
struct ixor_expr;
struct ior_expr;
struct land_expr;
struct lor_expr;
struct assign_expr;
struct expression;

x3::rule<primary, ast::expr> const primary("literal");
x3::rule<call_expr, ast::expr> const call_expr("expression");
x3::rule<pre_sinop_expr, ast::expr> const pre_sinop_expr("expression");
x3::rule<post_sinop_expr, ast::expr> const post_sinop_expr("expression");
x3::rule<mul_expr, ast::expr> const mul_expr("expression");
x3::rule<add_expr, ast::expr> const add_expr("expression");
x3::rule<shift_expr, ast::expr> const shift_expr("expression");
x3::rule<cmp_expr, ast::expr> const cmp_expr("expression");
x3::rule<iand_expr, ast::expr> const iand_expr("expression");
x3::rule<ixor_expr, ast::expr> const ixor_expr("expression");
x3::rule<ior_expr, ast::expr> const ior_expr("expression");
x3::rule<land_expr, ast::expr> const land_expr("expression");
x3::rule<lor_expr, ast::expr> const lor_expr("expression");
x3::rule<assign_expr, ast::expr> const assign_expr("expression");
x3::rule<expression, ast::expr> const expression("expression");

auto const primary_def =
    x3::int_[detail::assign()] | x3::bool_[detail::assign()] |
    ('"' >> x3::lexeme[*(x3::char_ - '"')] >> '"')[detail::assign_str()] |
    x3::raw[x3::lexeme[x3::alpha > *x3::alnum]][detail::assign_var()] |
    ("[" > expression % "," > "]")[detail::assign_as<ast::array>()] |
    ("{" > expression[detail::assign()] % x3::char_(";") > x3::lit(";") >
     "}")[detail::assign_as<ast::function>()] |
    ("(" > expression > ")")[detail::assign()];

auto const call_expr_def =
    primary[detail::assign()] >>
    *(("(" > expression > ")")[detail::assign_binop<ast::call>()] |
      ("[" > expression > "]")[detail::assign_binop<ast::at>()]);

auto const post_sinop_expr_def =
    call_expr[detail::assign()] |
    (call_expr > "++")[detail::assign_sinop<ast::add>(1)] |
    (call_expr > "--")[detail::assign_sinop<ast::sub>(1)];

auto const pre_sinop_expr_def =
    post_sinop_expr[detail::assign()] |
    ("!" > post_sinop_expr)[detail::assign_sinop<ast::ixor>(1)] |
    ("~" > post_sinop_expr)[detail::assign_sinop<ast::ixor>(1)] |
    ("++" > post_sinop_expr)[detail::assign_sinop<ast::add>(1)] |
    ("--" > post_sinop_expr)[detail::assign_sinop<ast::sub>(1)] |
    ("*" > post_sinop_expr)[detail::assign_sinop<ast::load>(1)];

auto const
    mul_expr_def = pre_sinop_expr[detail::assign()] >>
                   *(("*" > pre_sinop_expr)[detail::assign_binop<ast::mul>()] |
                     ("/" > pre_sinop_expr)[detail::assign_binop<ast::div>()]);

auto const add_expr_def = mul_expr[detail::assign()] >>
                          *(("+" > mul_expr)[detail::assign_binop<ast::add>()] |
                            ("-" > mul_expr)[detail::assign_binop<ast::sub>()] |
                            ("%" > mul_expr)[detail::assign_binop<ast::rem>()]);

auto const
    shift_expr_def = add_expr[detail::assign()] >>
                     *((">>" > add_expr)[detail::assign_binop<ast::shr>()] |
                       ("<<" > add_expr)[detail::assign_binop<ast::shl>()]);

auto const
    cmp_expr_def = shift_expr[detail::assign()] >>
                   *((">" > shift_expr)[detail::assign_binop<ast::gt>()] |
                     ("<" > shift_expr)[detail::assign_binop<ast::lt>()] |
                     (">=" > shift_expr)[detail::assign_binop<ast::gtq>()] |
                     ("<=" > shift_expr)[detail::assign_binop<ast::ltq>()] |
                     ("==" > shift_expr)[detail::assign_binop<ast::eeq>()] |
                     ("!=" > shift_expr)[detail::assign_binop<ast::neq>()]);

auto const iand_expr_def = cmp_expr[detail::assign()] >>
                           *(((x3::lit("&") - "&&") >
                              cmp_expr)[detail::assign_binop<ast::iand>()]);

auto const ixor_expr_def = iand_expr[detail::assign()] >>
                           *(("^" >
                              iand_expr)[detail::assign_binop<ast::ixor>()]);

auto const ior_expr_def = ixor_expr[detail::assign()] >>
                          *(((x3::lit("|") - "||") >
                             ixor_expr)[detail::assign_binop<ast::ior>()]);

auto const land_expr_def = ior_expr[detail::assign()] >>
                           *(("&&" >
                              ior_expr)[detail::assign_binop<ast::land>()]);

auto const lor_expr_def = land_expr[detail::assign()] >>
                          *(("||" >
                             land_expr)[detail::assign_binop<ast::lor>()]);

auto const assign_expr_def =
    lor_expr[detail::assign()] >> "=" >>
        assign_expr[detail::assign_binop<ast::assign>()] |
    lor_expr[detail::assign()];

auto const expression_def = assign_expr[detail::assign()];

BOOST_SPIRIT_DEFINE(primary, call_expr, pre_sinop_expr, post_sinop_expr,
                    mul_expr, shift_expr, cmp_expr, add_expr, iand_expr,
                    ixor_expr, ior_expr, land_expr, lor_expr, assign_expr,
                    expression);

struct expression {
  //  Our error handler
  template <typename Iterator, typename Exception, typename Context>
  x3::error_handler_result on_error(Iterator const &first, Iterator const &last,
                                    Exception const &x,
                                    Context const &context) {
    throw std::runtime_error(
        x.which() + " is expected but there is " +
        (x.where() == last ? "nothing" : std::string(x.where(), last)) + "\n" +
        std::string(first, x.where()) + "\033[1;33m" +
        std::string(x.where(), last) + "\033[1;0m");
    return x3::error_handler_result::fail;
  }
};
} // namespace grammar

ast::expr parse(std::string const &code) {
  ast::expr tree;

  auto const space_comment = x3::space | "/*" > *(x3::char_ - "*/") > "*/";
  if (!x3::phrase_parse(code.begin(), code.end(), grammar::expression,
                        space_comment, tree)) {
    throw std::runtime_error("detected error");
  }

  return tree;
}

}; // namespace parser
}; // namespace scopion
