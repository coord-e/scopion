/**
* @file scopc.cpp
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

#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <llvm/ADT/Triple.h>
#include <llvm/Support/Host.h>

#include "args.hxx"
#include "rang.hpp"

#include "scopion/scopion.hpp"

enum class OutputType { IR, AST, Assembly, Object };

static std::string getTmpFilePath()
{
  char tmpname[] = "/tmp/tmpfileXXXXXX";
  mkstemp(tmpname);
  return std::string(tmpname);
}

int main(int argc, char* argv[])
{
  args::ArgumentParser parser("scopc: scopion compiler", "");
  args::HelpFlag help(parser, "help", "Print this help", {'h', "help"});
  args::MapFlag<std::string, OutputType> type(parser, "type", "Specify the type of output",
                                              {'t', "type"},
                                              {{"ir", OutputType::IR},
                                               {"ast", OutputType::AST},
                                               {"asm", OutputType::Assembly},
                                               {"obj", OutputType::Object}},
                                              OutputType::Object);
  args::ValueFlag<std::string> output_path(parser, "path", "Specify the output path",
                                           {'o', "output"}, "./a.out");
  args::ValueFlag<std::string> arch(parser, "triple", "Specify the target triple", {'a', "arch"},
                                    "native");
  args::ValueFlag<int> optimize(parser, "level", "Set optimization level (0-3)", {'O', "optimize"},
                                3);
  args::Flag version(parser, "version", "Print version", {'V', "version"});
  args::Positional<std::string> input_path(parser, "path", "File to compile");

  parser.helpParams.addDefault = true;
  parser.helpParams.addChoices = true;

  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Help) {
    std::cout << parser;
    return 0;
  } catch (args::ParseError e) {
    std::cerr << rang::style::reset << rang::bg::red << rang::fg::gray << "[ERROR]"
              << rang::style::reset << ": " << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  } catch (args::ValidationError e) {
    std::cerr << rang::style::reset << rang::bg::red << rang::fg::gray << "[ERROR]"
              << rang::style::reset << ": " << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }

  if (version) {
    std::cerr << rang::style::reset << rang::fg::green <<
        R"(
     _______.  ______   ______   .______    __    ______   .__   __.
    /       | /      | /  __  \  |   _  \  |  |  /  __  \  |  \ |  |
   |   (----`|  ,----'|  |  |  | |  |_)  | |  | |  |  |  | |   \|  |
    \   \    |  |     |  |  |  | |   ___/  |  | |  |  |  | |  . `  |
.----)   |   |  `----.|  `--'  | |  |      |  | |  `--'  | |  |\   |
|_______/     \______| \______/  | _|      |__|  \______/  |__| \__|)"
              << rang::style::reset << std::endl
              << std::endl
              << rang::style::bold << rang::fg::green << "[scopc]" << rang::style::reset
              << ": scopion compiler" << std::endl
              << "Version: " << SCOPION_VERSION << std::endl
              << "Git: " << SCOPION_COMPILED_COMMIT_HASH << " on " << SCOPION_COMPILED_BRANCH
              << std::endl
              << "Compiled on: " << SCOPION_COMPILED_SYSTEM << std::endl;
    return 0;
  }

  if (!input_path) {
    std::cerr << rang::style::reset << rang::bg::red << rang::fg::gray << "[ERROR]"
              << rang::style::reset << ": no input file specified." << std::endl
              << parser;
    return 1;
  }

  auto outpath                   = args::get(output_path);
  outpath                        = outpath != "-" ? outpath : "/dev/stdout";
  boost::filesystem::path inpath = boost::filesystem::absolute(args::get(input_path));
  std::ifstream ifs(inpath.string());
  if (ifs.fail()) {
    std::cerr << rang::style::reset << rang::bg::red << rang::fg::gray << "[ERROR]"
              << rang::style::reset << ": failed to open \"" << inpath << "\"" << std::endl;
    return 0;
  }

  auto outtype = args::get(type);

  std::string code((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

  scopion::error err;
  auto ast = scopion::parser::parse(code, err, inpath);
  if (!ast) {
    std::cerr << err << std::endl;
    return -1;
  }

  if (outtype == OutputType::AST) {
    std::ofstream f(outpath);
    f << *ast << std::endl;
    f.close();
    return 0;
  }

  scopion::assembly::translator tr(inpath);
  tr.createMain();

  auto* tlv = tr.translateAST(*ast, err);
  if (!tlv) {
    std::cerr << err << std::endl;
    return -1;
  }

  if (!tr.createMainRet(tlv, err)) {
    std::cerr << err << std::endl;
    return -1;
  }

  auto mod = tr.takeModule();

  if (!mod->verify(err)) {
    std::cerr << err << std::endl;
    return -1;
  }

  if (uint8_t optlevel = args::get(optimize)) {
    mod->optimize(optlevel, optlevel);
  }

  auto irpath = outtype == OutputType::IR ? outpath : getTmpFilePath() + ".ll";
  std::ofstream f(irpath);
  mod->printIR(f);
  f.close();
  if (outtype == OutputType::IR)
    return 0;

  auto asmpath = outtype == OutputType::Assembly ? outpath : getTmpFilePath() + ".s";

  auto archstr = args::get(arch);
  archstr      = archstr != "native" ? archstr : llvm::sys::getDefaultTargetTriple();
  llvm::Triple triple(archstr);

  system(("llc -relocation-model=pic -mtriple=" + triple.getTriple() + " -filetype asm " + irpath +
          " -o=" + asmpath)
             .c_str());
  if (outtype == OutputType::Assembly)
    return 0;

  return system(("clang " + asmpath + " " + mod->generateLinkerFlags() +
                 " --target=" + triple.getTriple() + " -o " + outpath)
                    .c_str());
}
