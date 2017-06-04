#include "scopion/ast/ast.h"
#include "scopion/exceptions.h"
#include "scopion/parser/parser.h"

#include <boost/algorithm/string.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/deque.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/spirit/home/x3.hpp>

#include <array>

namespace scopion {
namespace parser {

namespace x3 = boost::spirit::x3;

namespace grammar {

namespace detail {

decltype(auto) assign() {
  return [](auto &&ctx) {
    x3::_val(ctx) = ast::expr(x3::_attr(ctx), x3::_where(ctx));
  };
}

decltype(auto) assign_str() {
  return [](auto &&ctx) {
    auto &&v = x3::_attr(ctx);
    x3::_val(ctx) = ast::expr(std::string(v.begin(), v.end()), x3::_where(ctx));
  };
}

template <typename T> decltype(auto) assign_as() {
  return [](auto &&ctx) {
    x3::_val(ctx) = ast::expr(T(x3::_attr(ctx)), x3::_where(ctx));
  };
}

decltype(auto) assign_func() {
  return [](auto &&ctx) {
    auto &&args = boost::fusion::at<boost::mpl::int_<0>>(x3::_attr(ctx));
    auto &&lines = boost::fusion::at<boost::mpl::int_<1>>(x3::_attr(ctx));
    std::vector<ast::variable> result;
    std::transform(
        args.begin(), args.end(), std::back_inserter(result), [](auto &&x) {
          return boost::get<ast::variable>(boost::get<ast::value>(x));
        });
    x3::_val(ctx) = ast::expr(ast::function(result, lines), x3::_where(ctx));
  };
}

decltype(auto) assign_var(bool rl = false) {
  return [rl](auto &&ctx) {
    auto &&v = x3::_attr(ctx);
    x3::_val(ctx) =
        ast::expr(ast::variable(std::string(v.begin(), v.end()), rl, false),
                  x3::_where(ctx));
  };
}

template <typename Op> decltype(auto) assign_binop() {
  return [](auto &&ctx) {
    x3::_val(ctx) = ast::expr(ast::binary_op<Op>(x3::_val(ctx), x3::_attr(ctx)),
                              x3::_where(ctx));
  };
}

struct make_op_lval : boost::static_visitor<ast::expr> {
  template <typename Op> ast::expr operator()(ast::binary_op<Op> op) const {
    op.lval = true;
    return op;
  }

  template <typename Op> ast::expr operator()(ast::single_op<Op> op) const {
    op.lval = true;
    return op;
  }

  ast::expr operator()(ast::value val) const {
    assert(false);
    return ast::expr();
  }
};

template <> decltype(auto) assign_binop<ast::assign>() {
  return [](auto &&ctx) {
    if (x3::_val(ctx).type() == typeid(ast::value)) {
      auto &&v = boost::get<ast::value>(x3::_val(ctx));
      if (v.type() == typeid(ast::variable)) {
        auto &&n = boost::get<ast::variable>(v).name;
        x3::_val(ctx) =
            ast::expr(ast::binary_op<ast::assign>(ast::variable(n, true, false),
                                                  x3::_attr(ctx)),
                      x3::_where(ctx));
        return;
      } else {
        x3::_pass(ctx) = false;
      }
    } else {
      x3::_val(ctx) =
          ast::expr(ast::binary_op<ast::assign>(
                        boost::apply_visitor(make_op_lval(), x3::_val(ctx)),
                        x3::_attr(ctx)),
                    x3::_where(ctx));
    }
  };
}

template <> decltype(auto) assign_binop<ast::call>() {
  return [](auto &&ctx) {
    if (x3::_val(ctx).type() != typeid(ast::value)) {
      x3::_val(ctx) = ast::binary_op<ast::call>(x3::_val(ctx),
                                                ast::arglist(x3::_attr(ctx)));
      return;
    }
    auto &&v = boost::get<ast::value>(x3::_val(ctx));
    if (v.type() == typeid(ast::variable)) {
      auto &&n = boost::get<ast::variable>(v).name;
      x3::_val(ctx) =
          ast::expr(ast::binary_op<ast::call>(ast::variable(n, false, true),
                                              ast::arglist(x3::_attr(ctx))),
                    x3::_where(ctx));
    } else if (v.type() == typeid(ast::function)) {
      x3::_val(ctx) =
          ast::expr(ast::binary_op<ast::call>(x3::_val(ctx),
                                              ast::arglist(x3::_attr(ctx))),
                    x3::_where(ctx));
    } else {
      x3::_pass(ctx) = false;
    }
  };
}

template <typename Op> decltype(auto) assign_sinop() {
  return [](auto &&ctx) {
    x3::_val(ctx) =
        ast::expr(ast::single_op<Op>(x3::_attr(ctx)), x3::_where(ctx));
  };
}

} // namespace detail

struct variable;
struct string;
struct array;
struct function;

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
struct ret_expr;
struct expression;

x3::rule<variable, ast::expr> const variable("variable");
x3::rule<string, ast::expr> const string("string");
x3::rule<array, ast::expr> const array("array");
x3::rule<function, ast::expr> const function("function");

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
x3::rule<ret_expr, ast::expr> const ret_expr("expression");
x3::rule<expression, ast::expr> const expression("expression");

auto const variable_def =
    x3::raw[x3::lexeme[x3::alpha > *x3::alnum]][detail::assign_var()];

auto const string_def =
    ('"' >> x3::lexeme[*(x3::char_ - '"')] >> '"')[detail::assign_str()];

auto const array_def = ("[" > *(expression >> -x3::lit(",")) >
                        "]")[detail::assign_as<ast::array>()];

auto const function_def = ((("(" > *(variable >> -x3::lit(","))) >> "){") >
                           *(expression >> ";") > "}")[detail::assign_func()];

auto const primary_def = x3::int_[detail::assign()] |
                         x3::bool_[detail::assign()] |
                         string[detail::assign()] | variable[detail::assign()] |
                         array[detail::assign()] | function[detail::assign()] |
                         ("(" >> expression >> ")")[detail::assign()];

auto const call_expr_def = primary[detail::assign()] >>
                           *(("(" > *(expression >> -x3::lit(",")) >
                              ")")[detail::assign_binop<ast::call>()] |
                             ("[" > expression >
                              "]")[detail::assign_binop<ast::at>()]);

auto const post_sinop_expr_def =
    (call_expr >> "++")[detail::assign_sinop<ast::inc>()] |
    (call_expr >> "--")[detail::assign_sinop<ast::dec>()] |
    call_expr[detail::assign()];

auto const pre_sinop_expr_def =
    post_sinop_expr[detail::assign()] |
    ("!" > post_sinop_expr)[detail::assign_sinop<ast::lnot>()] |
    ("~" > post_sinop_expr)[detail::assign_sinop<ast::inot>()] |
    ("++" > post_sinop_expr)[detail::assign_sinop<ast::inc>()] |
    ("--" > post_sinop_expr)[detail::assign_sinop<ast::dec>()] |
    ("*" > post_sinop_expr)[detail::assign_sinop<ast::load>()];

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

auto const cmp_expr_def =
    shift_expr[detail::assign()] >>
    *(((">" - x3::lit(">=")) > shift_expr)[detail::assign_binop<ast::gt>()] |
      (("<" - x3::lit("<=")) > shift_expr)[detail::assign_binop<ast::lt>()] |
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

auto const ret_expr_def =
    assign_expr[detail::assign()] |
    ("|>" > assign_expr)[detail::assign_sinop<ast::ret>()];

auto const expression_def = ret_expr[detail::assign()];

BOOST_SPIRIT_DEFINE(variable, string, array, function, primary, call_expr,
                    pre_sinop_expr, post_sinop_expr, mul_expr, shift_expr,
                    cmp_expr, add_expr, iand_expr, ixor_expr, ior_expr,
                    land_expr, lor_expr, assign_expr, ret_expr, expression);

struct expression {

  template <typename Iterator, typename Exception, typename Context>
  x3::error_handler_result on_error(Iterator const &first, Iterator const &last,
                                    Exception const &x,
                                    Context const &context) {
    throw general_error(
        x.which() + " is expected but there is " +
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
