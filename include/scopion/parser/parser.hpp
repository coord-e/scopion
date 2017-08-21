#ifndef SCOPION_PARSER_H_
#define SCOPION_PARSER_H_

#include "scopion/context.hpp"
#include "scopion/error.hpp"

#include "scopion/ast/ast.hpp"

#include <boost/optional.hpp>

namespace scopion
{
namespace parser
{
boost::optional<ast::expr> parse(context const& ctx, std::string const& filename, error& err);

};  // namespace parser

};  // namespace scopion

#endif  // SCOPION_PARSER_H_
