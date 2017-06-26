#include "gtest/gtest.h"

#include "scopion/scopion.hpp"

namespace {

using namespace scopion;

class parserTest : public ::testing::Test {};

TEST_F(parserTest, intVal) {
  EXPECT_EQ(parser::parse("(){1;}").ast,
            ast::expr(ast::function({{}, {ast::integer(1)}})));
}

TEST_F(parserTest, strVal) {
  EXPECT_EQ(parser::parse("(){\"test\";}").ast,
            ast::expr(ast::function({{}, {ast::string("test")}})));
}

TEST_F(parserTest, boolVal) {
  EXPECT_EQ(parser::parse("(){true;}").ast,
            ast::expr(ast::function({{}, {ast::boolean(true)}})));
}

TEST_F(parserTest, funcVal) {
  EXPECT_EQ(parser::parse("(){(){1;};}").ast,
            ast::expr(ast::function({{},
                                     std::vector<ast::expr>{ast::function(
                                         {{}, {ast::integer(1)}})}})));
}

TEST_F(parserTest, structVal) {
  EXPECT_EQ(parser::parse("(){[a:10,b:\"koko\"];}").ast,
            ast::expr(ast::function(
                {{},
                 std::vector<ast::expr>{ast::structure(
                     {{ast::identifier("a"), ast::integer(10)},
                      {ast::identifier("b"), ast::string("koko")}})}})));
}

TEST_F(parserTest, arrayVal) {
  EXPECT_EQ(
      parser::parse("(){[1,2,3];}").ast,
      ast::expr(ast::function({{},
                               {ast::array({ast::integer(1), ast::integer(2),
                                            ast::integer(3)})}})));
} // namespace

TEST_F(parserTest, addSubOp) {
  EXPECT_EQ(
      parser::parse("(){1+1;}").ast,
      ast::expr(ast::function(
          {{}, {ast::binary_op<ast::add>(ast::integer(1), ast::integer(1))}})));
  EXPECT_EQ(
      parser::parse("(){1-1;}").ast,
      ast::expr(ast::function(
          {{}, {ast::binary_op<ast::sub>(ast::integer(1), ast::integer(1))}})));
}

TEST_F(parserTest, mulDivOp) {
  EXPECT_EQ(
      parser::parse("(){1*1;}").ast,
      ast::expr(ast::function(
          {{}, {ast::binary_op<ast::mul>(ast::integer(1), ast::integer(1))}})));
  EXPECT_EQ(
      parser::parse("(){1/1;}").ast,
      ast::expr(ast::function(
          {{}, {ast::binary_op<ast::div>(ast::integer(1), ast::integer(1))}})));
}

TEST_F(parserTest, remOp) {
  EXPECT_EQ(
      parser::parse("(){1%1;}").ast,
      ast::expr(ast::function(
          {{}, {ast::binary_op<ast::rem>(ast::integer(1), ast::integer(1))}})));
}

TEST_F(parserTest, shiftOp) {
  EXPECT_EQ(
      parser::parse("(){1<<1;}").ast,
      ast::expr(ast::function(
          {{}, {ast::binary_op<ast::shl>(ast::integer(1), ast::integer(1))}})));
  EXPECT_EQ(
      parser::parse("(){1>>1;}").ast,
      ast::expr(ast::function(
          {{}, {ast::binary_op<ast::shr>(ast::integer(1), ast::integer(1))}})));
}

TEST_F(parserTest, andOrXorOp) {
  EXPECT_EQ(parser::parse("(){1&&1;}").ast,
            ast::expr(ast::function({{},
                                     {ast::binary_op<ast::land>(
                                         ast::integer(1), ast::integer(1))}})));
  EXPECT_EQ(
      parser::parse("(){1||1;}").ast,
      ast::expr(ast::function(
          {{}, {ast::binary_op<ast::lor>(ast::integer(1), ast::integer(1))}})));
  EXPECT_EQ(parser::parse("(){1^1;}").ast,
            ast::expr(ast::function({{},
                                     {ast::binary_op<ast::ixor>(
                                         ast::integer(1), ast::integer(1))}})));
  EXPECT_EQ(parser::parse("(){1&1;}").ast,
            ast::expr(ast::function({{},
                                     {ast::binary_op<ast::iand>(
                                         ast::integer(1), ast::integer(1))}})));
  EXPECT_EQ(
      parser::parse("(){1|1;}").ast,
      ast::expr(ast::function(
          {{}, {ast::binary_op<ast::ior>(ast::integer(1), ast::integer(1))}})));
}

TEST_F(parserTest, compareOp) {
  EXPECT_EQ(
      parser::parse("(){1==1;}").ast,
      ast::expr(ast::function(
          {{}, {ast::binary_op<ast::eeq>(ast::integer(1), ast::integer(1))}})));
  EXPECT_EQ(
      parser::parse("(){1!=1;}").ast,
      ast::expr(ast::function(
          {{}, {ast::binary_op<ast::neq>(ast::integer(1), ast::integer(1))}})));
  EXPECT_EQ(
      parser::parse("(){1>1;}").ast,
      ast::expr(ast::function(
          {{}, {ast::binary_op<ast::gt>(ast::integer(1), ast::integer(1))}})));
  EXPECT_EQ(
      parser::parse("(){1<1;}").ast,
      ast::expr(ast::function(
          {{}, {ast::binary_op<ast::lt>(ast::integer(1), ast::integer(1))}})));
  EXPECT_EQ(
      parser::parse("(){1>=1;}").ast,
      ast::expr(ast::function(
          {{}, {ast::binary_op<ast::gtq>(ast::integer(1), ast::integer(1))}})));
  EXPECT_EQ(
      parser::parse("(){1<=1;}").ast,
      ast::expr(ast::function(
          {{}, {ast::binary_op<ast::ltq>(ast::integer(1), ast::integer(1))}})));
}

TEST_F(parserTest, assignOp) {
  EXPECT_EQ(
      parser::parse("(){a=1;}").ast,
      ast::expr(ast::function(
          {{},
           {ast::binary_op<ast::assign>(ast::set_lval(ast::variable("a"), true),
                                        ast::integer(1))}})));
}

TEST_F(parserTest, sinOp) {
  EXPECT_EQ(parser::parse("(){|>1;}").ast,
            ast::expr(ast::function(
                {{}, {ast::single_op<ast::ret>(ast::integer(1))}})));
  EXPECT_EQ(parser::parse("(){!1;}").ast,
            ast::expr(ast::function(
                {{}, {ast::single_op<ast::lnot>(ast::integer(1))}})));
  EXPECT_EQ(parser::parse("(){~1;}").ast,
            ast::expr(ast::function(
                {{}, {ast::single_op<ast::inot>(ast::integer(1))}})));
  EXPECT_EQ(parser::parse("(){++1;}").ast,
            ast::expr(ast::function(
                {{}, {ast::single_op<ast::inc>(ast::integer(1))}})));
  EXPECT_EQ(parser::parse("(){--1;}").ast,
            ast::expr(ast::function(
                {{}, {ast::single_op<ast::dec>(ast::integer(1))}})));
  EXPECT_EQ(parser::parse("(){1++;}").ast,
            ast::expr(ast::function(
                {{}, {ast::single_op<ast::inc>(ast::integer(1))}})));
  EXPECT_EQ(parser::parse("(){1--;}").ast,
            ast::expr(ast::function(
                {{}, {ast::single_op<ast::dec>(ast::integer(1))}})));
  EXPECT_EQ(parser::parse("(){*1;}").ast,
            ast::expr(ast::function(
                {{}, {ast::single_op<ast::load>(ast::integer(1))}})));
}

TEST_F(parserTest, callOp) {
  EXPECT_EQ(
      parser::parse("(){a(1);}").ast,
      ast::expr(ast::function({{},
                               {ast::binary_op<ast::call>(
                                   ast::set_to_call(ast::variable("a"), true),
                                   ast::arglist({ast::integer(1)}))}})));
}

TEST_F(parserTest, atOp) {
  EXPECT_EQ(
      parser::parse("(){a[1];}").ast,
      ast::expr(ast::function(
          {{},
           {ast::binary_op<ast::at>(ast::variable("a"), ast::integer(1))}})));
}

TEST_F(parserTest, priority) {
  EXPECT_EQ(
      parser::parse("(){|>a=1||1&&1|1^1&1>1>>1+1**1++;}").ast,
      ast::expr(ast::function(
          {{},
           {ast::single_op<ast::ret>(ast::binary_op<ast::assign>(
               ast::set_lval(ast::variable("a"), true),
               ast::binary_op<ast::lor>(
                   ast::integer(1),
                   ast::binary_op<ast::land>(
                       ast::integer(1),
                       ast::binary_op<ast::ior>(
                           ast::integer(1),
                           ast::binary_op<ast::ixor>(
                               ast::integer(1),
                               ast::binary_op<ast::iand>(
                                   ast::integer(1),
                                   ast::binary_op<ast::gt>(
                                       ast::integer(1),
                                       ast::binary_op<ast::shr>(
                                           ast::integer(1),
                                           ast::binary_op<ast::add>(
                                               ast::integer(1),
                                               ast::binary_op<ast::mul>(
                                                   ast::integer(1),
                                                   ast::single_op<ast::load>(
                                                       ast::single_op<
                                                           ast::inc>(ast::integer(
                                                           1))))))))))))))}})));
}

} // namespace
