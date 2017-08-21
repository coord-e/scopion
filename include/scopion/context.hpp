#ifndef SCOPION_CONTEXT_H_
#define SCOPION_CONTEXT_H_

#include <llvm/IR/LLVMContext.h>
#include <string>

namespace scopion
{
struct context {
  llvm::LLVMContext llvmcontext;
  std::string code;

  context(std::string const& c) : code(c) {}
};

};  // namespace scopion

#endif
