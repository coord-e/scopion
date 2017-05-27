#include <cstdio>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "rang.hpp"

#include "AST/AST.h"
#include "Assembly/Assembly.h"
#include "Parser/Parser.h"
#include "exceptions.h"

#define SCOPION_VERSION "0.0.1-beta"

int main(int argc, char *argv[]) {
  std::cerr << "Welcome to scopion (" << SCOPION_VERSION << ")" << std::endl;
  std::string line;
  std::string newone;
  scopion::assembly::context ctx;
  while (1) {
    try {
      std::cout << "> ";
      std::string l;
      std::getline(std::cin, l);
      line += l + "\n";
      newone = "{\n" + line + "}";
      auto ast = scopion::parser::parse(newone);

      std::cout << "AST: " << ast.ast << std::endl;

      auto mod =
          scopion::assembly::module::create(ast, ctx, "scopion_interpreter");
      mod->run();
    } catch (scopion::general_error const &ex) {
      scopion::diagnosis e(ex);
      std::cerr << rang::style::reset << rang::bg::red << rang::fg::gray
                << "[ERROR]" << rang::style::reset << rang::fg::red << " @"
                << e.line_n() << rang::style::reset << ": " << e.what()
                << std::endl
                << e.line() << std::endl
                << rang::fg::green << std::setw(e.line_c() + 1) << "^"
                << rang::style::reset << std::endl;
    }
  }
}
