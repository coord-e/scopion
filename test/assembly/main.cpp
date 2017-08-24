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

#include "scopion/assembly/assembly.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/variant.hpp>

namespace
{
using namespace scopion;

class assemblyTest : public ::testing::Test
{
};

TEST_F(assemblyTest, variable)
{
  auto tree = ast::function(
      {{},
       {ast::binary_op<ast::assign>({ast::set_lval(ast::variable("test"), true), ast::integer(1)}),
        ast::single_op<ast::ret>(
            {ast::binary_op<ast::add>({ast::variable("test"), ast::integer(1)})})}});

  assembly::context ctx;
  auto res = scopion::assembly::module::create(parser::parsed(tree, ""), ctx, "testing")->irgen();
  auto str = R"(
define i32 @1() {
entry:
  %self = alloca i32 ()*
  store i32 ()* @1, i32 ()** %self
  %test = alloca i32
  store i32 1, i32* %test
  %0 = load i32, i32* %test
  %1 = load i32, i32* %test
  %2 = add i32 %1, 1
  ret i32 %2
}
)";
  EXPECT_EQ(str, res);
}

}  // namespace
