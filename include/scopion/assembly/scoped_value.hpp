#ifndef SCOPION_ASSEMBLY_VALUE_H_
#define SCOPION_ASSEMBLY_VALUE_H_

#include "scopion/ast/value.hpp"

#include <boost/variant.hpp>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Value.h>

#include <cassert>
#include <map>
#include <vector>

namespace scopion {
namespace assembly {

class scoped_value {
  llvm::Value *value_ = nullptr;
  llvm::BasicBlock *block_ = nullptr;
  std::vector<ast::expr> const *insts_ = nullptr;

public:
  std::map<std::string, scoped_value *> symbols;

  scoped_value() {}

  scoped_value(llvm::BasicBlock *block, std::vector<ast::expr> const *insts)
      : block_(block), insts_(insts) {}
  scoped_value(llvm::Value *val) : value_(val) {}
  scoped_value(llvm::Value *val, llvm::BasicBlock *block,
               std::vector<ast::expr> const *insts)
      : value_(val), block_(block), insts_(insts) {}

  llvm::Value *getValue() {
    if (value_ != nullptr)
      return value_;
    else if (block_ != nullptr)
      return block_;
    else
      return nullptr;
  }

  llvm::BasicBlock *getBlock() {
    assert(block_ != nullptr);
    return block_;
  }

  inline void setValue(llvm::Value *v) { value_ = v; }

  inline void setBlock(llvm::BasicBlock *b) { block_ = b; }

  std::vector<ast::expr> const *getInsts() {
    assert(insts_ != nullptr);
    return insts_;
  }

  llvm::Type *getType() {
    if (value_ != nullptr)
      return value_->getType();
    else if (block_ != nullptr)
      return block_->getType();
    else
      return nullptr;
  }

  operator llvm::Value *() { return getValue(); }

  operator llvm::BasicBlock *() { return getBlock(); }

  inline bool hasBlock() const { return block_ != nullptr; }

  inline bool hasValue() const { return value_ != nullptr; }
}; // namespace scopion
}; // namespace assembly
}; // namespace scopion

#endif
