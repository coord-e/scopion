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

template <typename Op> decltype(auto) assign_binop() {
  return [](auto &&ctx) {
    x3::_val(ctx) = ast::binary_op<Op>(x3::_val(ctx), x3::_attr(ctx));
  };
}

template <typename Op> decltype(auto) assign_sinop(ast::expr rv) {
  return [rv](auto &&ctx) {
    x3::_val(ctx) = ast::binary_op<Op>(x3::_attr(ctx), rv);
  };
}

} // namespace detail

struct primary;
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
struct expression;

x3::rule<primary, ast::expr> const primary;
x3::rule<pre_sinop_expr, ast::expr> const pre_sinop_expr;
x3::rule<post_sinop_expr, ast::expr> const post_sinop_expr;
x3::rule<mul_expr, ast::expr> const mul_expr;
x3::rule<add_expr, ast::expr> const add_expr;
x3::rule<shift_expr, ast::expr> const shift_expr;
x3::rule<cmp_expr, ast::expr> const cmp_expr;
x3::rule<iand_expr, ast::expr> const iand_expr;
x3::rule<ixor_expr, ast::expr> const ixor_expr;
x3::rule<ior_expr, ast::expr> const ior_expr;
x3::rule<land_expr, ast::expr> const land_expr;
x3::rule<lor_expr, ast::expr> const lor_expr;
x3::rule<expression, ast::expr> const expression;

auto const primary_def =
    x3::int_[detail::assign()] | x3::bool_[detail::assign()] |
    ('"' >> *(x3::char_ - '"') >> '"')[detail::assign_str()] |
    ("(" > expression > ")")[detail::assign()];

auto const post_sinop_expr_def =
    primary[detail::assign()] |
    (primary > "++")[detail::assign_sinop<ast::add>(1)] |
    (primary > "--")[detail::assign_sinop<ast::sub>(1)];

auto const pre_sinop_expr_def =
    post_sinop_expr[detail::assign()] |
    ("!" > post_sinop_expr)[detail::assign_sinop<ast::ixor>(1)] |
    ("~" > post_sinop_expr)[detail::assign_sinop<ast::ixor>(1)] |
    ("++" > post_sinop_expr)[detail::assign_sinop<ast::add>(1)] |
    ("--" > post_sinop_expr)[detail::assign_sinop<ast::sub>(1)];

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

auto const expression_def = lor_expr[detail::assign()];

BOOST_SPIRIT_DEFINE(primary, pre_sinop_expr, post_sinop_expr, mul_expr,
                    shift_expr, cmp_expr, add_expr, iand_expr, ixor_expr,
                    ior_expr, land_expr, lor_expr, expression);

} // namespace grammar

std::vector<ast::expr> parse(std::string const &code) {
  std::vector<std::string> result;
  boost::split(result, code, boost::is_any_of("\n;"));
  result.pop_back();

  std::vector<ast::expr> asts;

  int line = 0;
  bool err = false;
  for (auto const &i : result) {
    ast::expr tree;

    ++line;
    if (!x3::phrase_parse(i.begin(), i.end(), grammar::expression,
                          x3::ascii::space, tree)) {
      std::cerr << "@" << line << ": parse error" << std::endl;
      err = true;
      continue;
    }

    asts.push_back(tree);
  }

  if (err) {
    throw std::runtime_error("detected errors");
  }

  return asts;
}

ast::expr parse_line(std::string const &line) {
  ast::expr tree;

  if (!x3::phrase_parse(line.begin(), line.end(), grammar::expression,
                        x3::ascii::space, tree)) {
    std::cerr << "parse error" << std::endl;
    throw std::runtime_error("detected errors");
  }

  return tree;
}

}; // namespace parser
}; // namespace scopion
