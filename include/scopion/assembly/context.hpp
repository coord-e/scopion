#ifndef SCOPION_ASSEMBLY_CONTEXT_H_
#define SCOPION_ASSEMBLY_CONTEXT_H_

#include <llvm/IR/LLVMContext.h>

namespace scopion {
namespace assembly {

struct context {
  llvm::LLVMContext llvmcontext;
};

}; // namespace assembly
}; // namespace scopion

#endif
