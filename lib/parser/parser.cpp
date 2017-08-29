/**
* @file parser.cpp
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
#include <map>
#include <type_traits>

namespace scopion
{
namespace parser
{
namespace x3 = boost::spirit::x3;

namespace grammar
{
namespace detail
{
static std::string unescape(std::string const& s)
{
  std::string res;
  auto it = s.begin();
  while (it != s.end()) {
    char c = *it++;
    if (c == '\\' && it != s.end()) {
      switch (*it++) {
        case '\\':
          c = '\\';
          break;
        /*case '"':
          c = '"';
          break;
        case '\'':
          c = '\'';
          break;*/
        case 'n':
          c = '\n';
          break;
        case 't':
          c = '\t';
          break;
        case 'b':
          c = '\b';
          break;
        case 'f':
          c = '\f';
          break;
        case 'r':
          c = '\r';
          break;
        case 'v':
          c = '\v';
          break;
        case 'a':
          c = '\a';
          break;
        default:
          c = *(it - 1);
          break;
      }
    }
    res += c;
  }

  return res;
}

static auto const assign = [](auto&& ctx) { x3::_val(ctx) = x3::_attr(ctx); };

template <typename T>
static auto const assign_str_as = [](auto&& ctx) {
  std::string str(x3::_attr(ctx).begin(), x3::_attr(ctx).end());
  x3::_val(ctx) = ast::set_where(T(str), x3::_where(ctx));
};

template <typename T, char C, bool Es = false>
static auto const assign_str_escaped_as = [](auto&& ctx) {
  std::string rs;
  for (auto const& x : x3::_attr(ctx)) {
    if (char ch = boost::get<char>(x))
      rs += ch;
    else  // unused_type
      rs = rs + C;
  }
  x3::_val(ctx) = ast::set_where(T(Es ? unescape(rs) : rs), x3::_where(ctx));
};

template <typename T>
static auto const assign_as =
    [](auto&& ctx) { x3::_val(ctx) = ast::set_where(T(x3::_attr(ctx)), x3::_where(ctx)); };

static auto const assign_func = [](auto&& ctx) {
  auto&& args  = boost::fusion::at<boost::mpl::int_<0>>(x3::_attr(ctx));
  auto&& lines = boost::fusion::at<boost::mpl::int_<1>>(x3::_attr(ctx));
  std::vector<ast::identifier> result;
  std::transform(args.begin(), args.end(), std::back_inserter(result),
                 [](auto&& x) { return ast::unpack<ast::identifier>(x); });
  x3::_val(ctx) = ast::set_where(ast::function({result, lines}), x3::_where(ctx));
};

static auto const assign_struct = [](auto&& ctx) {
  std::map<ast::struct_key, ast::expr> result;
  for (auto const& v : x3::_attr(ctx)) {
    auto name    = ast::unpack<ast::struct_key>(boost::fusion::at<boost::mpl::int_<0>>(v));
    auto&& val   = boost::fusion::at<boost::mpl::int_<1>>(v);
    result[name] = val;
  }
  x3::_val(ctx) = ast::set_where(ast::structure(result), x3::_where(ctx));
};

template <typename Op, size_t N>
static auto const assign_op = [](auto&& ctx) {
  static_assert(N >= 3, "Wrong number of terms");
  std::array<ast::expr, N> ary;
  auto it = ary.begin();
  *it     = x3::_val(ctx);
  boost::fusion::for_each(x3::_attr(ctx), [&it](auto&& v) { *(++it) = v; });
  x3::_val(ctx) = ast::set_where(ast::op<Op, N>(ary), x3::_where(ctx));
};

template <typename Op>
static auto const assign_op<Op, 2> = [](auto&& ctx) {
  x3::_val(ctx) =
      ast::set_where(ast::binary_op<Op>({x3::_val(ctx), x3::_attr(ctx)}), x3::_where(ctx));
};

template <typename Op>
static auto const assign_op<Op, 1> = [](auto&& ctx) {
  x3::_val(ctx) = ast::set_where(ast::single_op<Op>({x3::_attr(ctx)}), x3::_where(ctx));
};

template <>
static auto const assign_op<ast::assign, 2> = [](auto&& ctx) {
  x3::_val(ctx) = ast::set_where(
      ast::binary_op<ast::assign>({ast::set_lval(x3::_val(ctx), true), x3::_attr(ctx)}),
      x3::_where(ctx));
};

template <>
static auto const assign_op<ast::call, 2> = [](auto&& ctx) {
  x3::_val(ctx) = ast::set_where(ast::binary_op<ast::call>({ast::set_to_call(x3::_val(ctx), true),
                                                            ast::arglist(x3::_attr(ctx))}),
                                 x3::_where(ctx));
};

template <>
static auto const assign_op<ast::inc, 1> = [](auto&& ctx) {
  x3::_val(ctx) = ast::set_where(
      ast::binary_op<ast::assign>({ast::set_lval(x3::_attr(ctx), true),
                                   ast::binary_op<ast::add>({x3::_attr(ctx), ast::integer(1)})}),
      x3::_where(ctx));
};

template <>
static auto const assign_op<ast::dec, 1> = [](auto&& ctx) {
  x3::_val(ctx) = ast::set_where(
      ast::binary_op<ast::assign>({ast::set_lval(x3::_attr(ctx), true),
                                   ast::binary_op<ast::sub>({x3::_attr(ctx), ast::integer(1)})}),
      x3::_where(ctx));
};

static auto const assign_attr = [](auto&& ctx) {
  auto&& key = boost::fusion::at<boost::mpl::int_<0>>(x3::_attr(ctx));
  // ast::identifier
  auto&& val = boost::fusion::at<boost::mpl::int_<1>>(x3::_attr(ctx));
  // ast::attribute_val
  std::string keystr = ast::val(ast::unpack<ast::identifier>(key));
  std::string valstr = val ? ast::val(ast::unpack<ast::attribute_val>(*val)) : "";
  x3::_val(ctx)      = ast::set_attr(x3::_val(ctx), keystr, valstr);
};

};  // namespace detail

struct variable;
struct pre_variable;
struct identifier;
struct struct_key;
struct string;
struct raw_string;
struct array;
struct structure;
struct function;
struct scope;

struct attribute_val;

struct primary;
struct attr_expr;
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
struct dot_expr;
struct assign_expr;
struct ret_expr;
struct expression;

x3::real_parser<double, x3::strict_real_policies<double>> const strict_double;

x3::rule<variable, ast::expr> const variable("variable");
x3::rule<pre_variable, ast::expr> const pre_variable("pre-defined variable");
x3::rule<struct_key, ast::expr> const struct_key("struct key");
x3::rule<identifier, ast::expr> const identifier("identifier");
x3::rule<string, ast::expr> const string("string");
x3::rule<raw_string, ast::expr> const raw_string("raw string");
x3::rule<array, ast::expr> const array("array");
x3::rule<structure, ast::expr> const structure("structure");
x3::rule<function, ast::expr> const function("function");
x3::rule<scope, ast::expr> const scope("scope");

x3::rule<attribute_val, ast::expr> const attribute_val("attribute val");

x3::rule<primary, ast::expr> const primary("literal");
x3::rule<attr_expr, ast::expr> const attr_expr("expression");
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
x3::rule<dot_expr, ast::expr> const dot_expr("expression");
x3::rule<assign_expr, ast::expr> const assign_expr("expression");
x3::rule<ret_expr, ast::expr> const ret_expr("expression");
x3::rule<expression, ast::expr> const expression("expression");

auto const identifier_p = x3::alpha > *(x3::alnum | '_');

auto const identifier_def =
    x3::raw[x3::lexeme[identifier_p]][detail::assign_str_as<ast::identifier>] >>
    *("#" >> identifier >> -(":" > attribute_val))[detail::assign_attr];

auto const variable_def = x3::raw[x3::lexeme[identifier_p]][detail::assign_str_as<ast::variable>];

auto const string_def = ('"' >> x3::lexeme[*(x3::lit("\\\"") | x3::char_ - '"')] >>
                         '"')[detail::assign_str_escaped_as<ast::string, '"', true>];

auto const raw_string_def = ('\'' >> x3::lexeme[*(x3::lit("\\'") | x3::char_ - '\'')] >>
                             '\'')[detail::assign_str_escaped_as<ast::string, '\''>];

auto const pre_variable_def =
    (x3::raw[x3::lexeme['@' > identifier_p]])[detail::assign_str_as<ast::pre_variable>];

auto const array_def = ("[" > *(expression >> -x3::lit(",")) > "]")[detail::assign_as<ast::array>];

auto const struct_key_def =
    x3::raw[x3::lexeme[identifier_p | x3::char_("+\\-*/%<>&|^~!") | (x3::char_("!=<>") > '=') |
                       x3::repeat(2)[x3::char_("+\\-<>&|")] |
                       "[]" |
                       "()"]][detail::assign_str_as<ast::struct_key>];

auto const structure_def =
    ("[" >> *(struct_key >> ":" >> expression >> -x3::lit(",")) >> "]")[detail::assign_struct];

auto const function_def = ((("(" > *(identifier >> -x3::lit(","))) >> ")" >> "{") >
                           *(expression >> ";") > "}")[detail::assign_func];

auto const scope_def = ("{" > *(expression >> ";") > "}")[detail::assign_as<ast::scope>];

auto const attribute_val_def = x3::raw[x3::lexeme[*(x3::alnum | x3::char_("_\\-./*[]{}"))]]
                                      [detail::assign_str_as<ast::attribute_val>];

auto const primary_def =
    strict_double[detail::assign_as<ast::decimal>] | x3::int_[detail::assign_as<ast::integer>] |
    x3::bool_[detail::assign_as<ast::boolean>] | string[detail::assign] |
    raw_string[detail::assign] | variable[detail::assign] | pre_variable[detail::assign] |
    structure[detail::assign] | array[detail::assign] | function[detail::assign] |
    scope[detail::assign] | ("(" >> expression >> ")")[detail::assign];

auto const dot_expr_def = primary[detail::assign] >>
                          *((".:" > struct_key)[detail::assign_op<ast::odot, 2>] |
                            (".=" > struct_key)[detail::assign_op<ast::adot, 2>] |
                            ("." >> struct_key)[detail::assign_op<ast::dot, 2>]);

auto const attr_expr_def = dot_expr[detail::assign] >>
                           *("#" >> identifier >> -(":" > attribute_val))[detail::assign_attr];

auto const call_expr_def = attr_expr[detail::assign] >>
                           *(("(" > *(expression >> -x3::lit(",")) >
                              ")")[detail::assign_op<ast::call, 2>] |
                             ("[" > expression > "]")[detail::assign_op<ast::at, 2>]);

auto const post_sinop_expr_def = (call_expr >> "++")[detail::assign_op<ast::inc, 1>] |
                                 (call_expr >> "--")[detail::assign_op<ast::dec, 1>] |
                                 call_expr[detail::assign];

auto const pre_sinop_expr_def = post_sinop_expr[detail::assign] |
                                ("!" > post_sinop_expr)[detail::assign_op<ast::lnot, 1>] |
                                ("~" > post_sinop_expr)[detail::assign_op<ast::inot, 1>] |
                                ("++" > post_sinop_expr)[detail::assign_op<ast::inc, 1>] |
                                ("--" > post_sinop_expr)[detail::assign_op<ast::dec, 1>];

auto const mul_expr_def = pre_sinop_expr[detail::assign] >>
                          *(("*" > pre_sinop_expr)[detail::assign_op<ast::mul, 2>] |
                            ("/" > pre_sinop_expr)[detail::assign_op<ast::div, 2>]);

auto const add_expr_def = mul_expr[detail::assign] >>
                          *(("+" > mul_expr)[detail::assign_op<ast::add, 2>] |
                            ("-" > mul_expr)[detail::assign_op<ast::sub, 2>] |
                            ("%" > mul_expr)[detail::assign_op<ast::rem, 2>]);

auto const shift_expr_def = add_expr[detail::assign] >>
                            *((">>" > add_expr)[detail::assign_op<ast::shr, 2>] |
                              ("<<" > add_expr)[detail::assign_op<ast::shl, 2>]);

auto const cmp_expr_def = shift_expr[detail::assign] >>
                          *(((">" - x3::lit(">=")) > shift_expr)[detail::assign_op<ast::gt, 2>] |
                            (("<" - x3::lit("<=")) > shift_expr)[detail::assign_op<ast::lt, 2>] |
                            (">=" > shift_expr)[detail::assign_op<ast::gtq, 2>] |
                            ("<=" > shift_expr)[detail::assign_op<ast::ltq, 2>] |
                            ("==" > shift_expr)[detail::assign_op<ast::eeq, 2>] |
                            ("!=" > shift_expr)[detail::assign_op<ast::neq, 2>]);

auto const iand_expr_def = cmp_expr[detail::assign] >>
                           *(((x3::lit("&") - "&&") > cmp_expr)[detail::assign_op<ast::iand, 2>]);

auto const ixor_expr_def = iand_expr[detail::assign] >>
                           *(("^" > iand_expr)[detail::assign_op<ast::ixor, 2>]);

auto const ior_expr_def = ixor_expr[detail::assign] >>
                          *(((x3::lit("|") - "||") > ixor_expr)[detail::assign_op<ast::ior, 2>]);

auto const land_expr_def = ior_expr[detail::assign] >>
                           *(("&&" > ior_expr)[detail::assign_op<ast::land, 2>]);

auto const lor_expr_def = land_expr[detail::assign] >>
                          *(("||" > land_expr)[detail::assign_op<ast::lor, 2>]);

auto const cond_expr_def = lor_expr[detail::assign] >>
                           *(("?" > lor_expr > ":" > lor_expr)[detail::assign_op<ast::cond, 3>]);

auto const assign_expr_def =
    cond_expr[detail::assign] >> "=" >> assign_expr[detail::assign_op<ast::assign, 2>] |
    cond_expr[detail::assign];

auto const ret_expr_def =
    assign_expr[detail::assign] | ("|>" > assign_expr)[detail::assign_op<ast::ret, 1>];

auto const expression_def = ret_expr[detail::assign];

BOOST_SPIRIT_DEFINE(pre_variable,
                    variable,
                    identifier,
                    struct_key,
                    string,
                    raw_string,
                    array,
                    structure,
                    function,
                    scope,
                    attribute_val,
                    primary,
                    dot_expr,
                    attr_expr,
                    call_expr,
                    pre_sinop_expr,
                    post_sinop_expr,
                    mul_expr,
                    shift_expr,
                    cmp_expr,
                    add_expr,
                    iand_expr,
                    ixor_expr,
                    ior_expr,
                    land_expr,
                    lor_expr,
                    cond_expr,
                    assign_expr,
                    ret_expr,
                    expression);

struct expression {
  template <typename Iterator, typename Exception, typename Context>
  x3::error_handler_result on_error(Iterator const& first,
                                    Iterator const& last,
                                    Exception const& x,
                                    Context const& context)
  {
    throw error(x.which() + " is expected but there is " +
                    (x.where() == last ? "nothing" : std::string{*x.where()}),
                boost::make_iterator_range(x.where(), x.where()),
                boost::make_iterator_range(first, last));
    return x3::error_handler_result::fail;
  }
};
};  // namespace grammar

parsed parse(std::string const& code)
{
  ast::expr tree;

  auto const space_comment =
      "//" > *(x3::char_ - '\n') > '\n' | "/*" > *(x3::char_ - "*/") > "*/" | x3::space;
  if (!x3::phrase_parse(code.begin(), code.end(), grammar::expression, space_comment, tree)) {
    throw std::runtime_error("detected error");
  }

  return parsed(tree, code);
}

};  // namespace parser
};  // namespace scopion
