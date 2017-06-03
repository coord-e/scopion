#include "gtest/gtest.h"

#include "scopion/scopion.h"

namespace {

using namespace scopion;

class parserTest : public ::testing::Test {};

TEST_F(parserTest, intVal) {
  EXPECT_EQ(parser::parse("{1;}").ast, ast::expr(ast::function({1})));
}

TEST_F(parserTest, strVal) {
  EXPECT_EQ(parser::parse("{\"test\";}").ast,
            ast::expr(ast::function({std::string("test")})));
}

TEST_F(parserTest, boolVal) {
  EXPECT_EQ(parser::parse("{true;}").ast, ast::expr(ast::function({true})));
}

TEST_F(parserTest, funcVal) {
  EXPECT_EQ(
      parser::parse("{{1;};}").ast,
      ast::expr(ast::function(std::vector<ast::expr>{ast::function({1})})));
}

TEST_F(parserTest, arrayVal) {
  EXPECT_EQ(parser::parse("{[1,2,3];}").ast,
            ast::expr(ast::function({ast::array({1, 2, 3})})));
}

TEST_F(parserTest, addSubOp) {
  EXPECT_EQ(parser::parse("{1+1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::add>(1, 1)})));
  EXPECT_EQ(parser::parse("{1-1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::sub>(1, 1)})));
}

TEST_F(parserTest, mulDivOp) {
  EXPECT_EQ(parser::parse("{1*1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::mul>(1, 1)})));
  EXPECT_EQ(parser::parse("{1/1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::div>(1, 1)})));
}

TEST_F(parserTest, remOp) {
  EXPECT_EQ(parser::parse("{1%1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::rem>(1, 1)})));
}

TEST_F(parserTest, shiftOp) {
  EXPECT_EQ(parser::parse("{1<<1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::shl>(1, 1)})));
  EXPECT_EQ(parser::parse("{1>>1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::shr>(1, 1)})));
}

TEST_F(parserTest, andOrXorOp) {
  EXPECT_EQ(parser::parse("{1&&1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::land>(1, 1)})));
  EXPECT_EQ(parser::parse("{1||1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::lor>(1, 1)})));
  EXPECT_EQ(parser::parse("{1^1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::ixor>(1, 1)})));
  EXPECT_EQ(parser::parse("{1&1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::iand>(1, 1)})));
  EXPECT_EQ(parser::parse("{1|1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::ior>(1, 1)})));
}

TEST_F(parserTest, compareOp) {
  EXPECT_EQ(parser::parse("{1==1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::eeq>(1, 1)})));
  EXPECT_EQ(parser::parse("{1!=1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::neq>(1, 1)})));
  EXPECT_EQ(parser::parse("{1>1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::gt>(1, 1)})));
  EXPECT_EQ(parser::parse("{1<1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::lt>(1, 1)})));
  EXPECT_EQ(parser::parse("{1>=1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::gtq>(1, 1)})));
  EXPECT_EQ(parser::parse("{1<=1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::ltq>(1, 1)})));
}

TEST_F(parserTest, assignOp) {
  EXPECT_EQ(parser::parse("{a=1;}").ast,
            ast::expr(ast::function({ast::binary_op<ast::assign>(
                ast::variable("a", true, false), 1)})));
}

TEST_F(parserTest, sinOp) {
  EXPECT_EQ(parser::parse("{|>1;}").ast,
            ast::expr(ast::function({ast::single_op<ast::ret>(1)})));
  EXPECT_EQ(parser::parse("{!1;}").ast,
            ast::expr(ast::function({ast::single_op<ast::lnot>(1)})));
  EXPECT_EQ(parser::parse("{~1;}").ast,
            ast::expr(ast::function({ast::single_op<ast::inot>(1)})));
  EXPECT_EQ(parser::parse("{++1;}").ast,
            ast::expr(ast::function({ast::single_op<ast::inc>(1)})));
  EXPECT_EQ(parser::parse("{--1;}").ast,
            ast::expr(ast::function({ast::single_op<ast::dec>(1)})));
  EXPECT_EQ(parser::parse("{1++;}").ast,
            ast::expr(ast::function({ast::single_op<ast::inc>(1)})));
  EXPECT_EQ(parser::parse("{1--;}").ast,
            ast::expr(ast::function({ast::single_op<ast::dec>(1)})));
  EXPECT_EQ(parser::parse("{*1;}").ast,
            ast::expr(ast::function({ast::single_op<ast::load>(1)})));
}

TEST_F(parserTest, callOp) {
  EXPECT_EQ(parser::parse("{a(1);}").ast,
            ast::expr(ast::function({ast::binary_op<ast::call>(
                ast::variable("a", false, true), 1)})));
}

TEST_F(parserTest, atOp) {
  EXPECT_EQ(parser::parse("{a[1];}").ast,
            ast::expr(ast::function({ast::binary_op<ast::at>(
                ast::variable("a", false, false), 1)})));
}

TEST_F(parserTest, priority) {
  EXPECT_EQ(
      parser::parse("{|>a=1||1&&1|1^1&1>1>>1+1**1++;}").ast,
      ast::expr(
          ast::function({ast::single_op<ast::ret>(ast::binary_op<ast::assign>(
              ast::variable("a", true, false),
              ast::binary_op<ast::lor>(
                  1,
                  ast::binary_op<ast::land>(
                      1,
                      ast::binary_op<ast::ior>(
                          1,
                          ast::binary_op<ast::ixor>(
                              1,
                              ast::binary_op<ast::iand>(
                                  1,
                                  ast::binary_op<ast::gt>(
                                      1,
                                      ast::binary_op<ast::shr>(
                                          1,
                                          ast::binary_op<ast::add>(
                                              1,
                                              ast::binary_op<ast::mul>(
                                                  1,
                                                  ast::single_op<ast::load>(
                                                      ast::single_op<ast::inc>(
                                                          1)))))))))))))})));
}

} // namespace
