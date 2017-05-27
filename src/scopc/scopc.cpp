#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "AST/AST.h"
#include "Assembly/Assembly.h"
#include "Parser/Parser.h"
#include "exceptions.h"

int main(int argc, char *argv[]) {
  try {
    if (argc != 3) {
      throw std::runtime_error("invalid arguments");
    }

    const std::string outbin(argv[2]);

    std::ifstream ifs(argv[1]);
    if (ifs.fail()) {
      std::cerr << "Failed to open: " << argv[1] << std::endl;
      throw std::runtime_error("Failed to start parsing.");
    }

    std::string code((std::istreambuf_iterator<char>(ifs)),
                     std::istreambuf_iterator<char>());

    auto ast = scopion::parser::parse(code);

    scopion::assembly::context ctx;
    auto mod = scopion::assembly::module::create(ast, ctx, outbin);

    char *tmpname = strdup("/tmp/tmpfileXXXXXX");
    mkstemp(tmpname);
    std::string tmpstr(tmpname);
    std::ofstream f(tmpstr);
    f << mod->irgen();
    f.close();
    system(("llc " + tmpstr).c_str());
    system(("gcc " + tmpstr + ".s -o " + std::string(outbin)).c_str());
  } catch (scopion::general_error const &ex) {
    scopion::diagnosis e(ex);
    std::cerr << "\033[1;31m[ERROR]\033[0m: @" << e.line_n() << ", " << e.what()
              << std::endl
              << e.line() << std::endl
              << std::string(e.line_c(), ' ') << "^" << std::endl;
  }
}
