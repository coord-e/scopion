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
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/IPO/Inliner.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

namespace scopion {
namespace assembly {

std::unique_ptr<module> module::create(parser::parsed const &tree, context &ctx,
                                       std::string const &name) {

  std::shared_ptr<llvm::Module> mod(new llvm::Module(name, ctx.llvmcontext));
  llvm::IRBuilder<> builder(mod->getContext());

  std::vector<llvm::Type *> args_type = {builder.getInt32Ty()};
  auto main_func = llvm::Function::Create(
      llvm::FunctionType::get(builder.getInt32Ty(), args_type, false),
      llvm::Function::ExternalLinkage, "main", mod.get());

  translator tr(mod, builder, tree.code);

  builder.SetInsertPoint(
      llvm::BasicBlock::Create(mod->getContext(), "main_entry", main_func));

  auto val = boost::apply_visitor(tr, tree.ast);

  // builder.CreateCall(val->getValue(), llvm::ArrayRef<llvm::Value *>({}));

  builder.CreateRet(builder.getInt32(0));

  return std::unique_ptr<module>(new module(mod, val));
}

std::string module::irgen() {
  std::string result;
  llvm::raw_string_ostream stream(result);

  module_->print(stream, nullptr);
  return result;
}

void module::optimize(uint8_t optLevel, uint8_t sizeLevel) {
  llvm::legacy::PassManager *pm = new llvm::legacy::PassManager();
  llvm::legacy::FunctionPassManager *fpm =
      new llvm::legacy::FunctionPassManager(module_.get());
  llvm::PassManagerBuilder builder;
  builder.OptLevel = optLevel;
  builder.SizeLevel = sizeLevel;
  builder.Inliner = llvm::createFunctionInliningPass(optLevel, sizeLevel);
  builder.DisableUnitAtATime = false;
  builder.DisableUnrollLoops = false;
  builder.LoopVectorize = true;
  builder.SLPVectorize = true;
  builder.populateModulePassManager(*pm);
  builder.populateFunctionPassManager(*fpm);
  pm->run(*module_);

  fpm->doInitialization();
  for (auto &f : module_->getFunctionList()) {
    fpm->run(f);
    f.optForSize();
  }
  fpm->doFinalization();
}

}; // namespace assembly
}; // namespace scopion
