#include "gtest/gtest.h"

#include "scopion/assembly/assembly.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/variant.hpp>

namespace {

using namespace scopion;

class assemblyTest : public ::testing::Test {};

TEST_F(assemblyTest, variable) {
  auto tree = ast::function(
      {ast::binary_op<ast::assign>(ast::variable("test", false, false), 1),
       ast::single_op<ast::ret>(
           ast::binary_op<ast::add>(ast::variable("test", true, false), 1),
           0)});

  assembly::context ctx;
  auto res = scopion::assembly::module::create(parser::parsed(tree, ""), ctx,
                                               "testing");
  std::string string;
  llvm::raw_string_ostream stream(string);
  res->val->print(stream);
  auto str = R"(
define i32 @1(i32) {
entry:
  %arg = alloca i32
  store i32 %0, i32* %arg
  %test = alloca i32
  store i32 1, i32* %test
  %1 = load i32, i32* %test
  %2 = load i32, i32* %test
  %3 = add i32 %2, 1
  ret i32 %3
}
)";
  EXPECT_EQ(str, stream.str());
}

} // namespace
