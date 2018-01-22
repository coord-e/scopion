/**
* @file evaluator.hpp
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

#ifndef SCOPION_ASSEMBLY_EVALUATOR_H_
#define SCOPION_ASSEMBLY_EVALUATOR_H_

#include "scopion/assembly/value.hpp"

#include "scopion/ast/expr.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include <memory>
#include <utility>

namespace scopion
{
namespace assembly
{
class translator;

std::pair<bool, value*> apply_bb(ast::scope const& sc, translator& tr);

struct evaluator : boost::static_visitor<value*> {
  value* v_;
  std::vector<value*> const& arguments_;
  translator& translator_;
  llvm::IRBuilder<>& builder_;

  evaluator(value* v, std::vector<value*> const& args, translator& tr);

  value* operator()(ast::value const&);
  value* operator()(ast::operators const&);

  value* operator()(ast::function const&);

  value* operator()(ast::scope const&);

  template <typename T>
  value* operator()(T const&)
  {
    // throw evaluate_no_support{};
    std::cerr << "T: " << typeid(T).name() << std::endl;
    assert(false && "Evaluating not supported type");
    return new value();  // void
  }
};

value* evaluate(value* v, std::vector<value*> const& args, translator& tr);
}
}

#endif
