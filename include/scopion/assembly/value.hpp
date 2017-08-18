#ifndef SCOPION_ASSEMBLY_VALUE_H_
#define SCOPION_ASSEMBLY_VALUE_H_

#include "scopion/ast/expr.hpp"
#include "scopion/ast/util.hpp"
#include "scopion/ast/value.hpp"

#include <boost/variant.hpp>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <cassert>
#include <map>
#include <vector>

namespace scopion
{
namespace assembly
{
class value
{
  llvm::Value* llvm_value_ = nullptr;
  value* parent_           = nullptr;
  ast::expr ast_value_;
  std::map<std::string, value*> symbols_;
  std::map<std::string, uint32_t> fields_;
  bool is_lazy_;
  bool is_void_;

public:
  value(llvm::Value* llvm_value, ast::expr ast_value, bool is_lazy = false)
      : llvm_value_(llvm_value), ast_value_(ast_value), is_lazy_(is_lazy), is_void_(false)
  {
  }
  value() : is_void_(true) {}

  value(value const&) = delete;
  value& operator=(value const&) = delete;

  value* copyWithNewLLVMValue(llvm::Value* v) const
  {
    assert(v != llvm_value_);
    auto newval         = new value();
    newval->llvm_value_ = v;
    newval->parent_     = parent_;
    newval->ast_value_  = ast_value_;
    newval->symbols_    = symbols_;
    newval->is_lazy_    = is_lazy_;
    newval->is_void_    = is_void_;
    newval->fields_  = fields_;
    return newval;
  }
  std::type_info const& type() const { return ast_value_.type(); }
  bool isLazy() const { return is_lazy_; }
  void isLazy(bool tf) { is_lazy_ = tf; }
  bool isFundamental() const
  {
    if (llvm_value_->getType()->isPointerTy())
      return !llvm_value_->getType()->getPointerElementType()->isStructTy();
    else
      return !llvm_value_->getType()->isStructTy();
  }
  bool isVoid() const { return llvm_value_ ? llvm_value_->getType()->isVoidTy() : is_void_; }
  value* getParent() const { return parent_; }
  void setParent(value* parent) { parent_ = parent; }
  ast::expr& getAst() { return ast_value_; }
  ast::expr const& getAst() const { return ast_value_; }
  llvm::Value* getLLVM() const { return llvm_value_; }
  void setLLVM(llvm::Value* const val) { llvm_value_ = val; }

  std::map<std::string, value*>& symbols() { return symbols_; }
  std::map<std::string, value*> const& symbols() const { return symbols_; }
  std::map<std::string, uint32_t>& fields() { return fields_; }
  std::map<std::string, uint32_t> const& fields() const { return fields_; }
};

};  // namespace assembly
};  // namespace scopion

#endif
