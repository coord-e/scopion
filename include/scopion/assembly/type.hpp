
/**
* @file type.hpp
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

#ifndef SCOPION_ASSEMBLY_TYPE_H_
#define SCOPION_ASSEMBLY_TYPE_H_

#include <llvm/IR/Type.h>

namespace scopion
{
namespace assembly
{
class type
{
  llvm::Type* llvm_type_ = nullptr;
  bool is_lazy_          = false;

  template <typename F>
  static bool isType_if_impl(llvm::Type* t, F f)
  {
    if (t->isPointerTy())
      return isType_if_impl(t->getPointerElementType(), f);
    return f(t);
  }

public:
  type(llvm::Type* llvm_type, bool is_lazy = false) : llvm_type_(llvm_type), is_lazy_(is_lazy) {}
  type() {}

  type(type const&) = delete;
  type& operator=(type const&) = delete;

  type* copy() const { return new type{llvm_type_, is_lazy_}; }

  bool isLazy() const { return is_lazy_; }
  llvm::Type* getLLVM() const { return llvm_type_; }
  void setLLVM(llvm::Type* ty) { llvm_type_ = ty; }

  bool isFundamental() const
  {
    return !isType_if_impl(llvm_type_, [](auto tp) { return tp->isStructTy() || tp->isArrayTy(); });
  }
  bool isStruct() const
  {
    return isType_if_impl(llvm_type_, [](auto tp) { return tp->isStructTy(); });
  }
  bool isArray() const
  {
    return isType_if_impl(llvm_type_, [](auto tp) { return tp->isArrayTy(); });
  }
  bool isVoid() const
  {
    if (llvm_type_)
      return llvm_type_->isVoidTy();
    else
      return true;
  }
};

};  // namespace assembly
};  // namespace scopion

#endif
