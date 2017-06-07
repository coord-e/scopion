#ifndef SCOPION_MODULE_H_
#define SCOPION_MODULE_H_

#include "scopion/assembly/context.hpp"
#include "scopion/assembly/translator.hpp"

#include "scopion/parser/parser.hpp"

#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/Module.h>

namespace scopion {
namespace assembly {

class module {
  std::unique_ptr<llvm::Module> module_;

public:
  llvm::Value *val;

  static std::unique_ptr<module> create(parser::parsed const &tree,
                                        context &ctx,
                                        std::string const &name = "");

  std::string irgen();
  llvm::GenericValue run();

private:
  module(std::unique_ptr<llvm::Module> &&module, llvm::Value *val_)
      : module_(std::move(module)), val(val_) {}
};

}; // namespace assembly
}; // namespace scopion

#endif
