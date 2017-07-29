#ifndef SCOPION_ASSEMBLY_TRANSLATOR_H_
#define SCOPION_ASSEMBLY_TRANSLATOR_H_

#include "scopion/assembly/value.hpp"
#include "scopion/ast/ast.hpp"

#include "scopion/error.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

namespace scopion {
namespace assembly {

class translator : public boost::static_visitor<value_t> {
  std::shared_ptr<llvm::Module> module_;
  llvm::IRBuilder<> &builder_;
  boost::iterator_range<std::string::const_iterator> const code_range_;
  value_t currentScope_;
  std::map<std::string, std::vector<std::string>> fields_map;
  std::map<std::string, value_t> lazy_map_;
  uint64_t structCount_ = 0;

public:
  translator(std::shared_ptr<llvm::Module> &module, llvm::IRBuilder<> &builder,
             std::string const &code);

  value_t operator()(ast::value value);
  value_t operator()(ast::operators value);

  value_t operator()(ast::integer value);
  value_t operator()(ast::boolean value);
  value_t operator()(ast::string const &value);
  value_t operator()(ast::variable const &value);
  value_t operator()(ast::array const &value);
  value_t operator()(ast::arglist const &value);
  value_t operator()(ast::structure const &value);
  value_t operator()(ast::function const &value);
  value_t operator()(ast::identifier const &value);
  value_t operator()(ast::scope const &value);

  template <class Op> value_t operator()(ast::single_op<Op> const &op) {
    return apply_op(op, boost::apply_visitor(*this, ast::val(op)[0]));
  }

  template <class Op> value_t operator()(ast::binary_op<Op> const &op) {
    value_t lhs = boost::apply_visitor(*this, ast::val(op)[0]);
    value_t rhs = boost::apply_visitor(*this, ast::val(op)[1]);

    return apply_op(op, lhs, rhs);
  }

  template <class Op> value_t operator()(ast::ternary_op<Op> const &op) {
    value_t first = boost::apply_visitor(*this, ast::val(op)[0]);
    value_t second = boost::apply_visitor(*this, ast::val(op)[1]);
    value_t third = boost::apply_visitor(*this, ast::val(op)[2]);

    return apply_op(op, first, second, third);
  }

private:
  std::map<std::string, value_t> &getSymbols(value_t &v) {
    if (v.type() == typeid(llvm::Value *))
      assert(false);
    if (v.type() == typeid(lazy_value<llvm::Function>))
      return boost::get<lazy_value<llvm::Function>>(v).symbols;
    else // if (v.type() == typeid(lazy_value<llvm::BasicBlock>))
      return boost::get<lazy_value<llvm::BasicBlock>>(v).symbols;
  }
  llvm::Value *get_v(value_t v) {
    if (v.type() == typeid(llvm::Value *)) {
      return boost::get<llvm::Value *>(v);
    } else {
      if (v.type() == typeid(lazy_value<llvm::Function>))
        return boost::get<lazy_value<llvm::Function>>(v).getValue();
      else // if (v.type() == typeid(lazy_value<llvm::BasicBlock>))
        return boost::get<lazy_value<llvm::BasicBlock>>(v).getValue();
    }
  }
  template <typename T> std::string getNameString(T *v) {
    std::string type_string;
    llvm::raw_string_ostream stream(type_string);
    v->print(stream);
    return stream.str();
  }
  inline std::string createNewBBName() {
    return "__BB_" +
           std::to_string(builder_.GetInsertBlock()->getParent()->size());
  }
  inline std::string createNewStructName() {
    return "__STRUCT_" + std::to_string(structCount_++);
  }
  template <typename T> std::string createAndRegisterLazyName(lazy_value<T> v) {
    std::string name = "__LAZY_" + std::to_string(lazy_map_.size());
    lazy_map_[name] = v;
    v->setName(name);
    return name;
  }
  value_t getLazyIfExists(llvm::Value *v) {
    try {
      return lazy_map_.at(v->getName());
    } catch (std::out_of_range &) {
      return v;
    }
  }

  bool apply_bb(lazy_value<llvm::BasicBlock> v);

  value_t apply_op(ast::binary_op<ast::add> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::sub> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::mul> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::div> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::rem> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::shl> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::shr> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::iand> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::ior> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::ixor> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::land> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::lor> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::eeq> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::neq> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::gt> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::lt> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::gtq> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::ltq> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::assign> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::call> const &, value_t lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::at> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::binary_op<ast::dot> const &, value_t const lhs,
                   value_t const rhs);
  value_t apply_op(ast::single_op<ast::load> const &, value_t const value);
  value_t apply_op(ast::single_op<ast::ret> const &, value_t const value);
  value_t apply_op(ast::single_op<ast::lnot> const &, value_t const value);
  value_t apply_op(ast::single_op<ast::inot> const &, value_t const value);
  value_t apply_op(ast::single_op<ast::inc> const &, value_t const value);
  value_t apply_op(ast::single_op<ast::dec> const &, value_t const value);
  value_t apply_op(ast::ternary_op<ast::cond> const &, value_t const first,
                   value_t const second, value_t const third);

  template <typename T>
  llvm::Value *apply_lazy(lazy_value<llvm::Function> value,
                          ast::value_wrapper<T> const &astv);
  template <typename T>
  llvm::Value *apply_lazy(lazy_value<llvm::BasicBlock> value,
                          ast::value_wrapper<T> const &astv);
};

}; // namespace assembly
}; // namespace scopion

#endif
