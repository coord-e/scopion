/**
* @file parser.hpp
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

#ifndef SCOPION_PARSER_H_
#define SCOPION_PARSER_H_

#include "scopion/ast/ast.hpp"

#include <boost/optional.hpp>

namespace scopion
{
namespace parser
{
boost::optional<ast::expr> parse(std::string const& code, error& err);

};  // namespace parser

};  // namespace scopion

#endif  // SCOPION_PARSER_H_
