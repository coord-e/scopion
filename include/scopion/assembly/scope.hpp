#ifndef SCOPION_ASSEMBLY_SCOPE_H_
#define SCOPION_ASSEMBLY_SCOPE_H_

#include <map>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Value.h>

namespace scopion {
namespace assembly {

class scope {
public:
  llvm::BasicBlock *block;
  std::map<std::string, llvm::Value *> symbols;

  scope(llvm::BasicBlock *block_) : block(block_) {}
};

}; // namespace assembly
}; // namespace scopion

#endif
