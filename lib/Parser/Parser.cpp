#include "scopion/ast/ast.hpp"
#include "scopion/error.hpp"
#include "scopion/parser/parser.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/deque.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <boost/spirit/home/x3.hpp>

#include <array>
#include <type_traits>

namespace scopion {
namespace parser {

namespace x3 = boost::spirit::x3;

namespace grammar {

namespace detail {

auto assign = [](auto &&ctx) { x3::_val(ctx) = x3::_attr(ctx); };

auto assign_str = [](auto &&ctx) {
  std::string str(x3::_attr(ctx).begin(), x3::_attr(ctx).end());
  x3::_val(ctx) = ast::set_where(ast::string(str), x3::_where(ctx));
};

template <typename T>
auto assign_as = [](auto &&ctx) {
  x3::_val(ctx) = ast::set_where(T(x3::_attr(ctx)), x3::_where(ctx));
};

auto assign_func = [](auto &&ctx) {
  auto &&args = boost::fusion::at<boost::mpl::int_<0>>(x3::_attr(ctx));
  auto &&lines = boost::fusion::at<boost::mpl::int_<1>>(x3::_attr(ctx));
  std::vector<ast::variable> result;
  std::transform(args.begin(), args.end(), std::back_inserter(result),
                 [](auto &&x) {
                   return boost::get<ast::variable>(boost::get<ast::value>(x));
                 });
  x3::_val(ctx) =
      ast::set_where(ast::function({result, lines}), x3::_where(ctx));
};

auto assign_var = [](auto &&ctx) {
  auto &&v = x3::_attr(ctx);
  x3::_val(ctx) = ast::set_where(ast::variable(std::string(v.begin(), v.end())),
                                 x3::_where(ctx));
};

template <typename Op, size_t N>
auto assign_op = [](auto &&ctx) {
  static_assert(N >= 3, "Wrong number of terms");
  std::array<ast::expr, N> ary;
  ary[0] = x3::_val(ctx);
  for (auto const &v : x3::_attr(ctx) | boost::adaptors::indexed()) {
    ary[v.index() + 1] = v.value();
  }
  x3::_val(ctx) = ast::set_where(ast::op<Op, N>(ary), x3::_where(ctx));
};

template <typename Op>
auto assign_op<Op, 2> = [](auto &&ctx) {
  x3::_val(ctx) = ast::set_where(
      ast::binary_op<Op>(ast::op_base<Op, 2>({x3::_val(ctx), x3::_attr(ctx)})),
      x3::_where(ctx));
};

template <typename Op>
auto assign_op<Op, 1> = [](auto &&ctx) {
  x3::_val(ctx) =
      ast::set_where(ast::single_op<Op>(ast::op_base<Op, 1>({x3::_attr(ctx)})),
                     x3::_where(ctx));
};

template <>
auto assign_op<ast::assign, 2> = [](auto &&ctx) {
  x3::_val(ctx) =
      ast::set_where(ast::binary_op<ast::assign>(ast::op_base<ast::assign, 2>(
                         {ast::set_lval(x3::_val(ctx), true), x3::_attr(ctx)})),
                     x3::_where(ctx));
};

template <>
auto assign_op<ast::call, 2> = [](auto &&ctx) {
  x3::_val(ctx) =
      ast::set_where(ast::binary_op<ast::call>(ast::op<ast::call, 2>(
                         {ast::set_to_call(x3::_val(ctx), true),
                          ast::arglist(x3::_attr(ctx))})),
                     x3::_where(ctx));
};

} // namespace detail

struct variable;
struct string;
struct array;
struct function;
struct scope;

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
struct cond_expr;
struct assign_expr;
struct ret_expr;
struct expression;

x3::rule<variable, ast::expr> const variable("variable");
x3::rule<string, ast::expr> const string("string");
x3::rule<array, ast::expr> const array("array");
x3::rule<function, ast::expr> const function("function");
x3::rule<scope, ast::expr> const scope("scope");

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
x3::rule<cond_expr, ast::expr> const cond_expr("expression");
x3::rule<assign_expr, ast::expr> const assign_expr("expression");
x3::rule<ret_expr, ast::expr> const ret_expr("expression");
x3::rule<expression, ast::expr> const expression("expression");

auto const variable_def =
    x3::raw[x3::lexeme[x3::alpha > *x3::alnum]][detail::assign_var];

auto const string_def =
    ('"' >> x3::lexeme[*(x3::char_ - '"')] >> '"')[detail::assign_str];

auto const array_def =
    ("[" > *(expression >> -x3::lit(",")) > "]")[detail::assign_as<ast::array>];

auto const function_def = ((("(" > *(variable >> -x3::lit(","))) >> "){") >
                           *(expression >> ";") > "}")[detail::assign_func];

auto const scope_def =
    ("{" > *(expression >> ";") > "}")[detail::assign_as<ast::scope>];

auto const primary_def = x3::int_[detail::assign_as<ast::integer>] |
                         x3::bool_[detail::assign_as<ast::boolean>] |
                         string[detail::assign] | variable[detail::assign] |
                         array[detail::assign] | function[detail::assign] |
                         scope[detail::assign] |
                         ("(" >> expression >> ")")[detail::assign];

auto const call_expr_def = primary[detail::assign] >>
                           *(("(" > *(expression >> -x3::lit(",")) >
                              ")")[detail::assign_op<ast::call, 2>] |
                             ("[" > expression >
                              "]")[detail::assign_op<ast::at, 2>]);

auto const post_sinop_expr_def =
    (call_expr >> "++")[detail::assign_op<ast::inc, 1>] |
    (call_expr >> "--")[detail::assign_op<ast::dec, 1>] |
    call_expr[detail::assign];

auto const pre_sinop_expr_def =
    post_sinop_expr[detail::assign] |
    ("!" > post_sinop_expr)[detail::assign_op<ast::lnot, 1>] |
    ("~" > post_sinop_expr)[detail::assign_op<ast::inot, 1>] |
    ("++" > post_sinop_expr)[detail::assign_op<ast::inc, 1>] |
    ("--" > post_sinop_expr)[detail::assign_op<ast::dec, 1>] |
    ("*" > post_sinop_expr)[detail::assign_op<ast::load, 1>];

auto const
    mul_expr_def = pre_sinop_expr[detail::assign] >>
                   *(("*" > pre_sinop_expr)[detail::assign_op<ast::mul, 2>] |
                     ("/" > pre_sinop_expr)[detail::assign_op<ast::div, 2>]);

auto const add_expr_def = mul_expr[detail::assign] >>
                          *(("+" > mul_expr)[detail::assign_op<ast::add, 2>] |
                            ("-" > mul_expr)[detail::assign_op<ast::sub, 2>] |
                            ("%" > mul_expr)[detail::assign_op<ast::rem, 2>]);

auto const
    shift_expr_def = add_expr[detail::assign] >>
                     *((">>" > add_expr)[detail::assign_op<ast::shr, 2>] |
                       ("<<" > add_expr)[detail::assign_op<ast::shl, 2>]);

auto const cmp_expr_def =
    shift_expr[detail::assign] >>
    *(((">" - x3::lit(">=")) > shift_expr)[detail::assign_op<ast::gt, 2>] |
      (("<" - x3::lit("<=")) > shift_expr)[detail::assign_op<ast::lt, 2>] |
      (">=" > shift_expr)[detail::assign_op<ast::gtq, 2>] |
      ("<=" > shift_expr)[detail::assign_op<ast::ltq, 2>] |
      ("==" > shift_expr)[detail::assign_op<ast::eeq, 2>] |
      ("!=" > shift_expr)[detail::assign_op<ast::neq, 2>]);

auto const iand_expr_def = cmp_expr[detail::assign] >>
                           *(((x3::lit("&") - "&&") >
                              cmp_expr)[detail::assign_op<ast::iand, 2>]);

auto const ixor_expr_def = iand_expr[detail::assign] >>
                           *(("^" >
                              iand_expr)[detail::assign_op<ast::ixor, 2>]);

auto const ior_expr_def = ixor_expr[detail::assign] >>
                          *(((x3::lit("|") - "||") >
                             ixor_expr)[detail::assign_op<ast::ior, 2>]);

auto const land_expr_def = ior_expr[detail::assign] >>
                           *(("&&" >
                              ior_expr)[detail::assign_op<ast::land, 2>]);

auto const lor_expr_def = land_expr[detail::assign] >>
                          *(("||" > land_expr)[detail::assign_op<ast::lor, 2>]);

auto const cond_expr_def = lor_expr[detail::assign] >>
                           *(("?" > lor_expr > ":" >
                              lor_expr)[detail::assign_op<ast::cond, 3>]);

auto const assign_expr_def =
    cond_expr[detail::assign] >> "=" >>
        assign_expr[detail::assign_op<ast::assign, 2>] |
    cond_expr[detail::assign];

auto const ret_expr_def = assign_expr[detail::assign] |
                          ("|>" > assign_expr)[detail::assign_op<ast::ret, 1>];

auto const expression_def = ret_expr[detail::assign];

BOOST_SPIRIT_DEFINE(variable, string, array, function, scope, primary,
                    call_expr, pre_sinop_expr, post_sinop_expr, mul_expr,
                    shift_expr, cmp_expr, add_expr, iand_expr, ixor_expr,
                    ior_expr, land_expr, lor_expr, cond_expr, assign_expr,
                    ret_expr, expression);

struct expression {

  template <typename Iterator, typename Exception, typename Context>
  x3::error_handler_result on_error(Iterator const &first, Iterator const &last,
                                    Exception const &x,
                                    Context const &context) {
    throw error(x.which() + " is expected but there is " +
                    (x.where() == last ? "nothing" : std::string{*x.where()}),
                boost::make_iterator_range(x.where(), x.where()),
                boost::make_iterator_range(first, last));
    return x3::error_handler_result::fail;
  }
};
}; // namespace grammar

parsed parse(std::string const &code) {
  ast::expr tree;

  auto const space_comment = x3::space | "/*" > *(x3::char_ - "*/") > "*/";
  if (!x3::phrase_parse(code.begin(), code.end(), grammar::expression,
                        space_comment, tree)) {
    throw std::runtime_error("detected error");
  }

  return parsed(tree, code);
}

}; // namespace parser
}; // namespace scopion
