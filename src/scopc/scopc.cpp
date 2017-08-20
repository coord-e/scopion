#include <cstdio>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <boost/filesystem.hpp>

#include <llvm/ADT/Triple.h>
#include <llvm/Support/Host.h>

#include "cmdline.h"
#include "rang.hpp"

#include "scopion/scopion.hpp"

namespace fs = boost::filesystem;

std::string getTmpFilePath()
{
  char* tmpname = strdup("/tmp/tmpfileXXXXXX");
  mkstemp(tmpname);
  return std::string(tmpname);
}

int main(int argc, char* argv[])
{
  try {
    cmdline::parser p;
    p.add<std::string>("type", 't', "Specify the type of output (ir, ast, asm, obj)", false, "obj",
                       cmdline::oneof<std::string>("ir", "ast", "asm", "obj"));
    p.add<std::string>("output", 'o', "Specify the output path", false, "./a.out");
    p.add<std::string>("arch", 'a', "Specify the target triple", false, "native");
    p.add<int>("optimize", 'O', "Enable optimization (1-3)", false, 1, cmdline::range(1, 3));
    p.add("help", 'h', "Print this help");
    p.add("version", 'v', "Print version");
    p.footer("filename ...");

    if (!p.parse(argc, argv)) {
      std::cout << rang::style::reset << rang::bg::red << rang::fg::gray << "[ERROR]"
                << rang::style::reset << ": " << p.error_full() << p.usage();
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
                << "scopion compiler version 0.1" << std::endl;
      return 0;
    }

    if (p.rest().empty()) {
      std::cout << rang::style::reset << rang::bg::red << rang::fg::gray << "[ERROR]"
                << rang::style::reset << ": no input file specified." << std::endl
                << p.usage();
      return 0;
    }

    std::string outpath = p.exist("output") ? p.get<std::string>("output") : "a.out";
    outpath             = outpath == "-" ? "/dev/stdout" : outpath;

    std::ifstream ifs(p.rest()[0]);
    if (ifs.fail()) {
      std::cout << rang::style::reset << rang::bg::red << rang::fg::gray << "[ERROR]"
                << rang::style::reset << ": failed to open \"" << p.rest()[0] << "\"" << std::endl;
      return 0;
    }

    auto& outtype = p.get<std::string>("type");

    std::string code((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    auto ast = scopion::parser::parse(code);

    if (outtype == "ast") {
      std::ofstream f(outpath);
      f << ast.ast;
      f.close();
      return 0;
    }

    scopion::assembly::context ctx;
    auto mod = scopion::assembly::module::create(ast, ctx, outpath);

    if (p.exist("optimize")) {
      auto opt = static_cast<uint8_t>(p.get<int>("optimize"));
      mod->optimize(opt, opt);
    }

    std::string ir = mod->irgen();

    auto irpath = outtype == "ir" ? outpath : getTmpFilePath() + ".ll";
    std::ofstream f(irpath);
    f << ir;
    f.close();
    if (outtype == "ir")
      return 0;

    auto asmpath = outtype == "asm" ? outpath : getTmpFilePath() + ".s";

    auto const archstr = p.get<std::string>("arch") != "native"
                             ? p.get<std::string>("arch")
                             : llvm::sys::getDefaultTargetTriple();
    llvm::Triple triple(archstr);

    system(("llc -mtriple=" + triple.getTriple() + " -filetype asm " + irpath + " -o=" + asmpath)
               .c_str());
    if (outtype == "asm")
      return 0;

    system(("gcc " + asmpath + " --target=" + triple.getTriple() + " -o " + outpath).c_str());
  } catch (scopion::error const& e) {
    std::cerr << rang::style::reset << rang::bg::red << rang::fg::gray << "[ERROR]"
              << rang::style::reset << rang::fg::red << " @" << e.line_number()
              << rang::style::reset << ": " << e.what() << std::endl
              << e.line() << std::endl
              << rang::fg::green << std::setw(e.distance() + 1) << "^" << rang::style::reset
              << std::endl;
    return -1;
  }
}
