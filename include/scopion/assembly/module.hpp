#ifndef SCOPION_ASSEMBLY_MODULE_H_
#define SCOPION_ASSEMBLY_MODULE_H_

#include "scopion/assembly/context.hpp"
#include "scopion/assembly/translator.hpp"

#include "scopion/parser/parser.hpp"

#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/Module.h>

namespace scopion {
namespace assembly {

class module {
  std::shared_ptr<llvm::Module> module_;

public:
  llvm::Value *val;

  static std::unique_ptr<module> create(parser::parsed const &tree,
                                        context &ctx,
                                        std::string const &name = "");

  std::string irgen();
  void optimize(uint8_t optLevel = 3, uint8_t sizeLevel = 0);
  llvm::GenericValue run();

private:
  module(std::shared_ptr<llvm::Module> &module, llvm::Value *val_)
      : module_(std::move(module)), val(val_) {}
};

}; // namespace assembly
}; // namespace scopion

#endif
