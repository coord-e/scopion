#ifndef SCOPION_ASSEMBLY_EVALUATOR_H_
#define SCOPION_ASSEMBLY_EVALUATOR_H_

#include "scopion/assembly/value.hpp"

#include "scopion/ast/expr.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include <memory>

namespace scopion
{
namespace assembly
{
class translator;

bool apply_bb(ast::scope const& sc, translator& tr);

struct evaluator : boost::static_visitor<llvm::Value*> {
  value* v_;
  std::vector<value*> const& arguments_;
  translator& translator_;
  llvm::IRBuilder<>& builder_;
  std::shared_ptr<llvm::Module>& module_;

  evaluator(value* v, std::vector<value*> const& args, translator& tr);

  llvm::Value* operator()(ast::value const&);
  llvm::Value* operator()(ast::operators const&);

  llvm::Value* operator()(ast::function const&);

  llvm::Value* operator()(ast::scope const&);

  template <typename T>
  llvm::Value* operator()(T const&)
  {
    // throw evaluate_no_support{};
    std::cerr << "T: " << typeid(T).name() << std::endl;
    assert(false && "Evaluating not supported type");
    return nullptr;  // void
  }
};

value* evaluate(value* v, std::vector<value*> const& args, translator& tr);
};
};

#endif
