/**
* @file value.hpp
*
* (c) copyright 2017 coord.e
*
* This file is part of scopion.
*
* scopion is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* scopion is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with scopion.  If not, see <http://www.gnu.org/licenses/>.
*/

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
class value;

using ret_table_t = std::pair<std::map<std::string, value*>, std::map<std::string, uint32_t>>;

class value
{
  llvm::Value* llvm_value_ = nullptr;
  value* parent_           = nullptr;
  ast::expr ast_value_;
  std::map<std::string, value*> symbols_;
  std::map<std::string, uint32_t> fields_;
  std::string name_;
  ret_table_t* ret_table_ = nullptr;
  bool is_lazy_;
  bool is_void_;

  template <typename F>
  static bool isType_if_impl(llvm::Type* t, F f)
  {
    if (t->isPointerTy())
      return isType_if_impl(t->getPointerElementType(), f);
    return f(t);
  }

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
    auto newval         = new value();
    newval->llvm_value_ = v;
    newval->parent_     = parent_;
    newval->ast_value_  = ast_value_;
    for (auto const& x : symbols_) {
      newval->symbols_[x.first] = x.second->copy();
    }
    newval->fields_    = fields_;
    newval->ret_table_ = ret_table_;
    newval->name_      = name_;
    newval->is_lazy_   = is_lazy_;
    newval->is_void_   = is_void_;
    return newval;
  }

  value* copy() { return copyWithNewLLVMValue(llvm_value_); }

  std::type_info const& type() const { return ast_value_.type(); }
  bool isLazy() const { return is_lazy_; }
  void isLazy(bool tf) { is_lazy_ = tf; }
  bool isFundamental() const
  {
    return !isType_if_impl(llvm_value_->getType(),
                           [](auto tp) { return tp->isStructTy() || tp->isArrayTy(); });
  }
  bool isStruct() const
  {
    return isType_if_impl(llvm_value_->getType(), [](auto tp) { return tp->isStructTy(); });
  }
  bool isArray() const
  {
    return isType_if_impl(llvm_value_->getType(), [](auto tp) { return tp->isArrayTy(); });
  }
  bool isVoid() const { return llvm_value_ ? llvm_value_->getType()->isVoidTy() : is_void_; }
  value* getParent() const { return parent_; }
  void setParent(value* parent) { parent_ = parent; }
  void setRetTable(ret_table_t* table) { ret_table_ = table; }
  ret_table_t* getRetTable() const { return ret_table_; }
  void applyRetTable(ret_table_t* table)
  {
    symbols_ = table->first;
    fields_  = table->second;
  }
  ret_table_t* generateRetTable()
  {
    auto ret_table    = new ret_table_t();
    ret_table->first  = symbols_;
    ret_table->second = fields_;
    return ret_table;
  }
  std::string getName() const { return name_; }
  void setName(std::string const& name) { name_ = name; }
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
