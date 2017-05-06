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
  while (1) {
    try {
      std::cout << "> ";
      std::getline(std::cin, line);
      scopion::assembly asmb("scopion_interpreter");
      auto ast = scopion::parser::parse_line(line);

      std::cout << "AST: ";
      boost::apply_visitor(scopion::ast::printer(std::cout), ast);
      std::cout << std::endl;

      asmb.IRGen({ast});
      char *tmpname = strdup("/tmp/tmpfileXXXXXX");
      mkstemp(tmpname);
      std::string tmpstr(tmpname);
      std::ofstream f(tmpstr);
      f << asmb.getIR();
      f.close();
      if (system(("llc " + tmpstr).c_str()) != 0)
        throw std::runtime_error("Error occured while compiling IR");
      if (system(("gcc " + tmpstr + ".s -o " + tmpstr + "exec").c_str()) != 0)
        throw std::runtime_error("Error occured while compiling assembly");
      if (system((tmpstr + "exec").c_str()) != 0)
        throw std::runtime_error("Error occured while executing");
    } catch (std::exception const &e) {
      std::cerr << "\033[1;31m[ERROR]\033[0m: " << e.what() << std::endl;
    }
  }
}
