/**
* @file module.hpp
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

#ifndef SCOPION_ASSEMBLY_MODULE_H_
#define SCOPION_ASSEMBLY_MODULE_H_

#include "scopion/assembly/translator.hpp"
#include "scopion/assembly/value.hpp"

#include "scopion/parser/parser.hpp"

#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

namespace scopion
{
namespace assembly
{
class module;

std::unique_ptr<module> translate(ast::expr const& tree, std::string const& name, error& err);

class module
{
  friend std::unique_ptr<module> translate(ast::expr const& tree,
                                           std::string const& name,
                                           error& err);

  std::shared_ptr<llvm::Module> llvm_module_;
  value* value_;

public:
  module(const module&) = delete;
  module& operator=(const module&) = delete;

  void printIR(std::ostream& os) const;
  std::string getPrintedIR() const;
  void optimize(uint8_t optLevel = 3, uint8_t sizeLevel = 0);

  value* getValue() const;

private:
  module(std::shared_ptr<llvm::Module>& module, value* val_);
};

};  // namespace assembly
};  // namespace scopion

#endif
