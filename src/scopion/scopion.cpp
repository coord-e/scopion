#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "AST/AST.h"
#include "Assembly/Assembly.h"
#include "Parser/Parser.h"

#define SCOPION_VERSION "0.0.1-beta"

int main(int argc, char *argv[]) {
  std::cerr << "Welcome to scopion (" << SCOPION_VERSION << ")" << std::endl;
  std::string line;
  scopion::assembly::context ctx;
  while (1) {
    try {
      std::cout << "> ";
      std::string l;
      std::getline(std::cin, l);
      line += l;
      auto ast = scopion::parser::parse("{" + line + "}");

      std::cout << "AST: " << ast << std::endl;

      auto mod =
          scopion::assembly::module::create(ast, ctx, "scopion_interpreter");
      mod->run();
    } catch (std::exception const &e) {
      std::cerr << "\033[1;31m[ERROR]\033[0m: " << e.what() << std::endl;
    }
  }
}
