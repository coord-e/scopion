/**
* @file module.cpp
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

#include "scopion/assembly/module.hpp"

#include "scopion/ast/ast.hpp"

#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/CallGraphSCCPass.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/RegionPass.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/IPO/Inliner.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#include <algorithm>
#include <numeric>
#include <string>

namespace scopion
{
namespace assembly
{
module::module(std::string const& name, std::string const& entry_function_name)
    : context_(new llvm::LLVMContext()),
      llvm_module_(new llvm::Module(name, *context_)),
      entry_function_name_(entry_function_name)
{
}

module::~module()
{
  delete llvm_module_;
}

void module::printIR(std::ostream& os) const
{
  llvm::raw_os_ostream stream(os);

  llvm_module_->print(stream, nullptr);
}

std::string module::getPrintedIR() const
{
  std::string result;
  llvm::raw_string_ostream stream(result);

  llvm_module_->print(stream, nullptr);
  return result;
}

std::string module::getEntryFunctionName() const
{
  return entry_function_name_;
}

void module::optimize(uint8_t optLevel, uint8_t sizeLevel)
{
  llvm::legacy::PassManager* pm          = new llvm::legacy::PassManager();
  llvm::legacy::FunctionPassManager* fpm = new llvm::legacy::FunctionPassManager(llvm_module_);
  llvm::PassManagerBuilder builder;
  builder.OptLevel           = optLevel;
  builder.SizeLevel          = sizeLevel;
  builder.Inliner            = llvm::createFunctionInliningPass(optLevel, sizeLevel, true);
  builder.DisableUnitAtATime = false;
  builder.DisableUnrollLoops = false;
  builder.LoopVectorize      = true;
  builder.SLPVectorize       = true;
  builder.populateModulePassManager(*pm);
  builder.populateFunctionPassManager(*fpm);
  pm->run(*llvm_module_);

  fpm->doInitialization();
  for (auto& f : llvm_module_->getFunctionList()) {
    fpm->run(f);
    f.optForSize();
  }
  fpm->doFinalization();
}

bool module::verify(error& err) const
{
  std::string result;
  llvm::raw_string_ostream stream(result);
  if (llvm::verifyModule(*llvm_module_, &stream)) {
    err = error("Invaild IR has generated\nIR:\n" + getPrintedIR(), locationInfo{}, errorType::Bug);
    return false;
  } else
    return true;
}

llvm::LLVMContext& module::getContext() const
{
  return llvm_module_->getContext();
}

llvm::Module* module::getLLVMModule() const
{
  return llvm_module_;
}

std::string module::generateLinkerFlags()
{
  std::sort(link_libraries_.begin(), link_libraries_.end());
  auto result = std::unique(link_libraries_.begin(), link_libraries_.end());
  return std::accumulate(
      link_libraries_.begin(), result, std::string{},
      [](auto const& str1, auto const& str2) { return str1 + "-l" + str2 + " "; });
  ;
}

}  // namespace assembly
}  // namespace scopion
