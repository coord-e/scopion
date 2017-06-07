#include "scopion/assembly/module.hpp"

#include "scopion/ast/ast.hpp"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/TargetSelect.h>

namespace scopion {
namespace assembly {

std::unique_ptr<module> module::create(parser::parsed const &tree, context &ctx,
                                       std::string const &name) {

  std::unique_ptr<llvm::Module> mod(new llvm::Module(name, ctx.llvmcontext));
  llvm::IRBuilder<> builder(mod->getContext());

  std::vector<llvm::Type *> args_type = {builder.getInt32Ty()};
  auto *main_func = llvm::Function::Create(
      llvm::FunctionType::get(builder.getInt32Ty(), args_type, false),
      llvm::Function::ExternalLinkage, "main", mod.get());

  builder.SetInsertPoint(
      llvm::BasicBlock::Create(mod->getContext(), "main_entry", main_func));

  translator tr(std::move(mod), builder, tree.code);
  auto val = boost::apply_visitor(tr, tree.ast);
  mod = tr.returnModule();

  builder.CreateCall(val, llvm::ArrayRef<llvm::Value *>({}));

  builder.CreateRet(builder.getInt32(0));

  return std::unique_ptr<module>(new module(std::move(mod), val));
}

std::string module::irgen() {
  std::string result;
  llvm::raw_string_ostream stream(result);

  module_->print(stream, nullptr);
  return result;
}

llvm::GenericValue module::run() {
  llvm::InitializeNativeTarget();
  auto *funcptr = llvm::cast<llvm::Function>(val);
  std::unique_ptr<llvm::ExecutionEngine> engine(
      llvm::EngineBuilder(std::move(module_))
          .setEngineKind(llvm::EngineKind::Either)
          .create());
  engine->finalizeObject();
  auto res = engine->runFunction(funcptr, std::vector<llvm::GenericValue>(1));
  engine->removeModule(module_.get());
  return res;
}

}; // namespace assembly
}; // namespace scopion
