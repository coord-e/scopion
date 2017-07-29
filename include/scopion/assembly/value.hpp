#ifndef SCOPION_ASSEMBLY_VALUE_H_
#define SCOPION_ASSEMBLY_VALUE_H_

#include "scopion/ast/value.hpp"

#include <boost/variant.hpp>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Value.h>

#include <cassert>
#include <map>
#include <vector>

namespace scopion
{
namespace assembly
{
template <typename T>
class lazy_value;

using value_t = boost::variant<lazy_value<llvm::Function>,
                               lazy_value<llvm::BasicBlock>,
                               llvm::Value*>;

template <typename T>
class lazy_value
{
  T* block_ = nullptr;
  std::vector<ast::expr> insts_;

public:
  std::map<std::string, value_t> symbols;

  lazy_value() {}
  lazy_value(T* block, std::vector<ast::expr> const insts)
      : block_(block), insts_(insts)
  {
  }

  void setValue(T* value) noexcept { block_ = value; }

  T* getValue() const noexcept { return block_; }

  auto getInsts() const noexcept { return insts_; }

  T* operator->() const noexcept { return block_; }
};

};  // namespace assembly
};  // namespace scopion

#endif
