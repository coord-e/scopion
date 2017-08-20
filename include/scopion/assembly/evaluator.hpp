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
  std::shared_ptr<llvm::Module>& module_;

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
};
};

#endif
