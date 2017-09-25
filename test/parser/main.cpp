/**
* @file main.cpp
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

#include "gtest/gtest.h"

#include "scopion/scopion.hpp"

namespace
{
using namespace scopion;

class parserTest : public ::testing::Test
{
};

ast::expr parseWithErrorHandling(std::string const& str)
{
  scopion::error err;
  if (auto res = parser::parse(str, err))
    return *res;
  else {
    std::cerr << err << std::endl;
    throw err;
  }
}

TEST_F(parserTest, intVal)
{
  EXPECT_EQ(parseWithErrorHandling("(){1;}"), ast::expr(ast::function({{}, {ast::integer(1)}})));
}

TEST_F(parserTest, strVal)
{
  EXPECT_EQ(parseWithErrorHandling("(){\"test\";}"),
            ast::expr(ast::function({{}, {ast::string("test")}})));
}

TEST_F(parserTest, boolVal)
{
  EXPECT_EQ(parseWithErrorHandling("(){true;}"),
            ast::expr(ast::function({{}, {ast::boolean(true)}})));
}

TEST_F(parserTest, funcVal)
{
  EXPECT_EQ(parseWithErrorHandling("(){(){1;};}"),
            ast::expr(ast::function(
                {{}, std::vector<ast::expr>{ast::function({{}, {ast::integer(1)}})}})));
}

TEST_F(parserTest, structVal)
{
  EXPECT_EQ(parseWithErrorHandling("(){[a:10,b:\"koko\"];}"),
            ast::expr(ast::function({{},
                                     std::vector<ast::expr>{ast::structure(
                                         {{ast::struct_key("a"), ast::integer(10)},
                                          {ast::struct_key("b"), ast::string("koko")}})}})));
}

TEST_F(parserTest, arrayVal)
{
  EXPECT_EQ(parseWithErrorHandling("(){[1,2,3];}"),
            ast::expr(ast::function(
                {{}, {ast::array({ast::integer(1), ast::integer(2), ast::integer(3)})}})));
}  // namespace

TEST_F(parserTest, addSubOp)
{
  EXPECT_EQ(parseWithErrorHandling("(){1+1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::add>({ast::integer(1), ast::integer(1)})}})));
  EXPECT_EQ(parseWithErrorHandling("(){1-1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::sub>({ast::integer(1), ast::integer(1)})}})));
}  // namespace

TEST_F(parserTest, mulDivOp)
{
  EXPECT_EQ(parseWithErrorHandling("(){1*1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::mul>({ast::integer(1), ast::integer(1)})}})));
  EXPECT_EQ(parseWithErrorHandling("(){1/1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::div>({ast::integer(1), ast::integer(1)})}})));
}

TEST_F(parserTest, remOp)
{
  EXPECT_EQ(parseWithErrorHandling("(){1%1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::rem>({ast::integer(1), ast::integer(1)})}})));
}

TEST_F(parserTest, shiftOp)
{
  EXPECT_EQ(parseWithErrorHandling("(){1<<1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::shl>({ast::integer(1), ast::integer(1)})}})));
  EXPECT_EQ(parseWithErrorHandling("(){1>>1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::shr>({ast::integer(1), ast::integer(1)})}})));
}

TEST_F(parserTest, andOrXorOp)
{
  EXPECT_EQ(parseWithErrorHandling("(){1&&1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::land>({ast::integer(1), ast::integer(1)})}})));
  EXPECT_EQ(parseWithErrorHandling("(){1||1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::lor>({ast::integer(1), ast::integer(1)})}})));
  EXPECT_EQ(parseWithErrorHandling("(){1^1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::ixor>({ast::integer(1), ast::integer(1)})}})));
  EXPECT_EQ(parseWithErrorHandling("(){1&1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::iand>({ast::integer(1), ast::integer(1)})}})));
  EXPECT_EQ(parseWithErrorHandling("(){1|1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::ior>({ast::integer(1), ast::integer(1)})}})));
}

TEST_F(parserTest, compareOp)
{
  EXPECT_EQ(parseWithErrorHandling("(){1==1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::eeq>({ast::integer(1), ast::integer(1)})}})));
  EXPECT_EQ(parseWithErrorHandling("(){1!=1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::neq>({ast::integer(1), ast::integer(1)})}})));
  EXPECT_EQ(parseWithErrorHandling("(){1>1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::gt>({ast::integer(1), ast::integer(1)})}})));
  EXPECT_EQ(parseWithErrorHandling("(){1<1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::lt>({ast::integer(1), ast::integer(1)})}})));
  EXPECT_EQ(parseWithErrorHandling("(){1>=1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::gtq>({ast::integer(1), ast::integer(1)})}})));
  EXPECT_EQ(parseWithErrorHandling("(){1<=1;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::ltq>({ast::integer(1), ast::integer(1)})}})));
}

TEST_F(parserTest, assignOp)
{
  EXPECT_EQ(parseWithErrorHandling("(){a=b=c;}"),
            ast::expr(ast::function(
                {{},
                 {ast::binary_op<ast::assign>(
                     {ast::set_lval(ast::variable("a"), true),
                      ast::binary_op<ast::assign>(
                          {ast::set_lval(ast::variable("b"), true), ast::variable("c")})})}})));
}

TEST_F(parserTest, sinOp)
{
  EXPECT_EQ(parseWithErrorHandling("(){|>1;}"),
            ast::expr(ast::function({{}, {ast::single_op<ast::ret>({ast::integer(1)})}})));
  EXPECT_EQ(parseWithErrorHandling("(){!1;}"),
            ast::expr(ast::function({{}, {ast::single_op<ast::lnot>({ast::integer(1)})}})));
  EXPECT_EQ(parseWithErrorHandling("(){~1;}"),
            ast::expr(ast::function({{}, {ast::single_op<ast::inot>({ast::integer(1)})}})));

  /*** parse-time AST replacement ***/
  EXPECT_EQ(parseWithErrorHandling("(){++2;}"),
            ast::expr(ast::function(
                {{},
                 {ast::binary_op<ast::assign>(
                     {ast::set_lval(ast::integer(2), true),
                      ast::binary_op<ast::add>({ast::integer(2), ast::integer(1)})})}})));
  EXPECT_EQ(parseWithErrorHandling("(){--2;}"),
            ast::expr(ast::function(
                {{},
                 {ast::binary_op<ast::assign>(
                     {ast::set_lval(ast::integer(2), true),
                      ast::binary_op<ast::sub>({ast::integer(2), ast::integer(1)})})}})));
  EXPECT_EQ(parseWithErrorHandling("(){2++;}"),
            ast::expr(ast::function(
                {{},
                 {ast::binary_op<ast::assign>(
                     {ast::set_lval(ast::integer(2), true),
                      ast::binary_op<ast::add>({ast::integer(2), ast::integer(1)})})}})));
  EXPECT_EQ(parseWithErrorHandling("(){2--;}"),
            ast::expr(ast::function(
                {{},
                 {ast::binary_op<ast::assign>(
                     {ast::set_lval(ast::integer(2), true),
                      ast::binary_op<ast::sub>({ast::integer(2), ast::integer(1)})})}})));
}

TEST_F(parserTest, callOp)
{
  EXPECT_EQ(parseWithErrorHandling("(){a(1);}"),
            ast::expr(ast::function(
                {{},
                 {ast::binary_op<ast::call>({ast::set_to_call(ast::variable("a"), true),
                                             ast::arglist({ast::integer(1)})})}})));
}

TEST_F(parserTest, atOp)
{
  EXPECT_EQ(parseWithErrorHandling("(){a[1];}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::at>({ast::variable("a"), ast::integer(1)})}})));
}

TEST_F(parserTest, dotOp)
{
  EXPECT_EQ(parseWithErrorHandling("(){a.b;}"),
            ast::expr(ast::function(
                {{}, {ast::binary_op<ast::dot>({ast::variable("a"), ast::struct_key("b")})}})));
  EXPECT_EQ(
      parseWithErrorHandling("(){a.:b();}"),
      ast::expr(ast::function(
          {{},
           {ast::binary_op<ast::call>(
               {ast::set_to_call(
                    ast::binary_op<ast::odot>({ast::variable("a"), ast::struct_key("b")}), true),
                ast::arglist()})}})));
  EXPECT_EQ(
      parseWithErrorHandling("(){a.=b();}"),
      ast::expr(ast::function(
          {{},
           {ast::binary_op<ast::call>(
               {ast::set_to_call(
                    ast::binary_op<ast::adot>({ast::variable("a"), ast::struct_key("b")}), true),
                ast::arglist()})}})));
}

TEST_F(parserTest, priority)
{
  EXPECT_EQ(
      parseWithErrorHandling("(){|>a=1||1&&1|1^1&1>1>>1+1*1++;}"),
      ast::expr(ast::function(
          {{},
           {ast::single_op<ast::ret>({ast::binary_op<ast::assign>(
               {ast::set_lval(ast::variable("a"), true),
                ast::binary_op<ast::lor>(
                    {ast::integer(1),
                     ast::binary_op<ast::land>(
                         {ast::integer(1),
                          ast::binary_op<ast::ior>(
                              {ast::integer(1),
                               ast::binary_op<ast::ixor>(
                                   {ast::integer(1),
                                    ast::binary_op<ast::iand>(
                                        {ast::integer(1),
                                         ast::binary_op<ast::gt>(
                                             {ast::integer(1),
                                              ast::binary_op<ast::shr>(
                                                  {ast::integer(1),
                                                   ast::binary_op<ast::add>(
                                                       {ast::integer(1),
                                                        ast::binary_op<ast::mul>(
                                                            {ast::integer(1),
                                                             ast::binary_op<ast::assign>(
                                                                 {ast::set_lval(ast::integer(1),
                                                                                true),
                                                                  ast::binary_op<ast::add>(
                                                                      {ast::integer(1),
                                                                       ast::integer(
                                                                           1)})})})})})})})})})})})})})}})));
}

TEST_F(parserTest, escapeSequence)
{
  EXPECT_EQ(parseWithErrorHandling(R"((){"\n\t\b\f\r\v\a\\\s\"";})"),
            ast::expr(ast::function({{}, {ast::string("\n\t\b\f\r\v\a\\s\"")}})));
  EXPECT_EQ(parseWithErrorHandling(R"((){'\n\t\b\f\r\v\a\s\'';})"),
            ast::expr(ast::function({{}, {ast::string("\\n\\t\\b\\f\\r\\v\\a\\s'")}})));
}
}
