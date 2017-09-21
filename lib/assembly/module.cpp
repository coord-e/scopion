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

namespace scopion
{
namespace assembly
{
std::unique_ptr<module> translate(ast::expr const& tree,
                                  error& err,
                                  boost::optional<boost::filesystem::path> const& path)
{
  auto ctx = new llvm::LLVMContext();
  std::shared_ptr<llvm::Module> mod(
      new llvm::Module(path ? path->filename().string() : "notafile", *ctx));
  llvm::IRBuilder<> builder(mod->getContext());

  std::vector<llvm::Type*> args_type = {builder.getInt32Ty(),
                                        builder.getInt8PtrTy()->getPointerTo()};
  auto main_func =
      llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), args_type, false),
                             llvm::Function::ExternalLinkage, "main", mod.get());

  std::vector<llvm::Value*> arg_llvm_values;
  std::vector<value*> arg_values;
  for (auto it = main_func->arg_begin(); it != main_func->arg_end(); it++) {
    arg_llvm_values.push_back(it);
    arg_values.push_back(new value(it, ast::expr{}));
  }

  auto mainbb = llvm::BasicBlock::Create(mod->getContext(), "main_entry", main_func);
  builder.SetInsertPoint(mainbb);

  translator tr(mod, builder);

  value* val;
  llvm::Value* etlv;
  try {
    val  = boost::apply_visitor(tr, tree);
    etlv = evaluate(val, arg_values, tr)->getLLVM();
  } catch (error& e) {
    err = e;
    return nullptr;
  }

  llvm::Value* ret = builder.CreateCall(etlv, llvm::ArrayRef<llvm::Value*>(arg_llvm_values));

  builder.CreateRet(ret->getType()->isVoidTy() ? builder.getInt32(0) : ret);

  if (tr.hasGCUsed()) {
    builder.SetInsertPoint(mainbb, mainbb->begin());
    tr.insertGCInit();
  }

  auto destv      = std::unique_ptr<module>(new module(mod, val));
  destv->gc_used_ = tr.hasGCUsed();

  llvm::raw_os_ostream stream(std::cerr);
  if (llvm::verifyModule(*mod, &stream)) {
    err = error("Invaild IR has generated\nIR:\n" + destv->getPrintedIR(), locationInfo{},
                errorType::Bug);
    return nullptr;
  }
  return destv;
}

module::module(std::shared_ptr<llvm::Module>& module, value* val_)
    : llvm_module_(module), value_(val_)
{
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

void module::optimize(uint8_t optLevel, uint8_t sizeLevel)
{
  llvm::legacy::PassManager* pm = new llvm::legacy::PassManager();
  llvm::legacy::FunctionPassManager* fpm =
      new llvm::legacy::FunctionPassManager(llvm_module_.get());
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

value* module::getValue() const
{
  return value_;
}

bool module::hasGCUsed() const
{
  return gc_used_;
}

};  // namespace assembly
};  // namespace scopion
