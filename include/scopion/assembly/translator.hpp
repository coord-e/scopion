#ifndef SCOPION_ASSEMBLY_TRANSLATOR_H_
#define SCOPION_ASSEMBLY_TRANSLATOR_H_

#include "scopion/assembly/evaluator.hpp"
#include "scopion/assembly/value.hpp"
#include "scopion/ast/ast.hpp"

#include "scopion/error.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

#include <utility>

namespace scopion
{
namespace assembly
{
template <typename T>
std::string getNameString(T* v)
{
  std::string type_string;
  llvm::raw_string_ostream stream(type_string);
  v->print(stream);
  return stream.str();
}

class translator : public boost::static_visitor<value*>
{
  std::shared_ptr<llvm::Module> module_;
  llvm::IRBuilder<>& builder_;
  boost::iterator_range<std::string::const_iterator> const code_range_;
  value* thisScope_;

  friend struct evaluator;
  friend std::pair<bool, value*> apply_bb(ast::scope const& sc, translator& tr);

public:
  translator(std::shared_ptr<llvm::Module>& module,
             llvm::IRBuilder<>& builder,
             std::string const& code);

  value* operator()(ast::value);
  value* operator()(ast::operators);

  value* operator()(ast::integer);
  value* operator()(ast::boolean);
  value* operator()(ast::string const&);
  value* operator()(ast::pre_variable const&);
  value* operator()(ast::variable const&);
  value* operator()(ast::array const&);
  value* operator()(ast::arglist const&);
  value* operator()(ast::structure const&);
  value* operator()(ast::function const&);
  value* operator()(ast::identifier const&);
  value* operator()(ast::struct_key const&);
  value* operator()(ast::scope const&);

  template <typename Op, size_t N>
  value* operator()(ast::op<Op, N> const& op)
  {
    auto it       = ast::val(op).begin();
    value* target = boost::apply_visitor(*this, *(it++));
    if (!target->isStruct())  // no its own opr method
      it--;
    std::vector<value*> args;
    std::vector<llvm::Value*> args_llvm;
    for (; it != ast::val(op).end(); it++) {
      if (ast::isa<ast::arglist>(*it) && target->isStruct()) {  // unpack arglist
        std::vector<ast::expr> al = ast::val(ast::unpack<ast::arglist>(*it));
        for (auto const& x : al) {
          auto thev = boost::apply_visitor(*this, x);

          args.push_back(thev);
          if (!thev->isLazy())
            args_llvm.push_back(thev->getLLVM());
        }
      } else {
        auto thev = it == ast::val(op).begin() ? target : boost::apply_visitor(*this, *it);

        args.push_back(thev);
        if (!thev->isLazy())
          args_llvm.push_back(thev->getLLVM());
      }
    }

    if (!target->isStruct()) {
      return apply_op(op, args);
    } else {
      args.push_back(target);
      args_llvm.push_back(target->getLLVM());
      auto f = target->symbols().find(ast::op_str<Op>);
      if (f == target->symbols().end())
        throw error(std::string("no operator ") + ast::op_str<Op> + " is defined in the structure",
                    ast::attr(op).where, code_range_);
      auto v         = evaluate(f->second, args, *this);
      auto ret_table = v->getRetTable();
      auto destv =
          new value(builder_.CreateCall(v->getLLVM(), llvm::ArrayRef<llvm::Value*>(args_llvm)), op);
      if (ret_table)
        destv->applyRetTable(ret_table);
      return destv;
    }
  }

  void setScope(value* v) { thisScope_ = v; }
  value* getScope() const { return thisScope_; }

private:
  llvm::Value* createGCMalloc(llvm::Type* Ty,
                              llvm::Value* ArraySize  = nullptr,
                              const llvm::Twine& Name = "");

  bool copyFull(value* src,
                value* dest,
                std::string const& name,
                llvm::Value* newv = nullptr,
                value* defp       = nullptr);
  llvm::Value* sizeofType(llvm::Type*);

  value* apply_op(ast::binary_op<ast::add> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::sub> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::mul> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::div> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::rem> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::shl> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::shr> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::iand> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::ior> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::ixor> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::land> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::lor> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::eeq> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::neq> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::gt> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::lt> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::gtq> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::ltq> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::assign> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::call> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::at> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::dot> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::odot> const&, std::vector<value*> const&);
  value* apply_op(ast::binary_op<ast::adot> const&, std::vector<value*> const&);
  value* apply_op(ast::single_op<ast::ret> const&, std::vector<value*> const&);
  value* apply_op(ast::single_op<ast::lnot> const&, std::vector<value*> const&);
  value* apply_op(ast::single_op<ast::inot> const&, std::vector<value*> const&);
  value* apply_op(ast::single_op<ast::inc> const&, std::vector<value*> const&);
  value* apply_op(ast::single_op<ast::dec> const&, std::vector<value*> const&);
  value* apply_op(ast::ternary_op<ast::cond> const&, std::vector<value*> const&);
};

};  // namespace assembly
};  // namespace scopion

#endif
