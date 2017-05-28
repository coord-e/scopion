#include <cstdio>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "cmdline.h"
#include "rang.hpp"

#include "scopion/scopion.h"

#define SCOPION_VERSION "0.0.1-beta"

int main(int argc, char *argv[]) {
  cmdline::parser p;
  p.add("help", 'h', "Print this help");
  p.add("version", 'v', "Print version");

  if (!p.parse(argc, argv)) {
    std::cout << rang::style::reset << rang::bg::red << rang::fg::gray
              << "[ERROR]" << rang::style::reset << ": " << p.error_full()
              << p.usage();
    return 0;
  }
  if (p.exist("help")) {
    std::cout << p.usage();
    return 0;
  }

  if (p.exist("version")) {
    std::cout << rang::style::reset << rang::fg::green <<
        R"(
     _______.  ______   ______   .______    __    ______   .__   __.
    /       | /      | /  __  \  |   _  \  |  |  /  __  \  |  \ |  |
   |   (----`|  ,----'|  |  |  | |  |_)  | |  | |  |  |  | |   \|  |
    \   \    |  |     |  |  |  | |   ___/  |  | |  |  |  | |  . `  |
.----)   |   |  `----.|  `--'  | |  |      |  | |  `--'  | |  |\   |
|_______/     \______| \______/  | _|      |__|  \______/  |__| \__|)"
              << rang::style::reset << std::endl
              << std::endl
              << "scopion interpreter version 0.1" << std::endl;
    return 0;
  }

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
