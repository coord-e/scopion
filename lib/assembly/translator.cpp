#include "scopion/assembly/translator.hpp"
#include "scopion/assembly/value.hpp"

#include "scopion/error.hpp"

#include <algorithm>
#include <map>
#include <string>

#include <boost/range/adaptor/indexed.hpp>
#include <boost/range/iterator_range.hpp>

namespace scopion
{
namespace assembly
{
translator::translator(std::shared_ptr<llvm::Module>& module,
                       llvm::IRBuilder<>& builder,
                       std::string const& code)
    : boost::static_visitor<value*>(),
      module_(module),
      builder_(builder),
      code_range_(boost::make_iterator_range(code.begin(), code.end())),
      thisScope_(new value())
{
  auto ib = builder_.GetInsertBlock();
  {
    std::vector<llvm::Type*> args = {builder_.getInt8Ty()->getPointerTo()};

    module_->getOrInsertFunction(
        "printf",
        llvm::FunctionType::get(builder_.getInt32Ty(), llvm::ArrayRef<llvm::Type*>(args), true));
    module_->getOrInsertFunction(
        "puts",
        llvm::FunctionType::get(builder_.getInt32Ty(), llvm::ArrayRef<llvm::Type*>(args), true));
  }
  {
    std::vector<llvm::Type*> func_args_type;
    func_args_type.push_back(builder_.getInt32Ty());

    llvm::FunctionType* llvm_func_type =
        llvm::FunctionType::get(builder_.getInt32Ty(), func_args_type, /*可変長引数=*/false);
    llvm::Function* llvm_func = llvm::Function::Create(
        llvm_func_type, llvm::Function::ExternalLinkage, "printn", module_.get());

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(module_->getContext(),
                                                       "entry_printn",  // BasicBlockの名前
                                                       llvm_func);
    builder_.SetInsertPoint(entry);  // entryの開始
    std::vector<llvm::Value*> args = {builder_.CreateGlobalStringPtr("%d\n"),
                                      &(*(llvm_func->getArgumentList().begin()))};
    builder_.CreateCall(module_->getFunction("printf"), args);
    builder_.CreateRet(llvm::ConstantInt::get(builder_.getInt32Ty(), 0, true));
  }
  builder_.SetInsertPoint(ib);
}

value* translator::operator()(ast::value astv)
{
  return boost::apply_visitor(*this, astv);
}

value* translator::operator()(ast::operators astv)
{
  return boost::apply_visitor(*this, astv);
}

value* translator::operator()(ast::integer astv)
{
  if (ast::attr(astv).lval)
    throw error("An integer constant is not to be assigned", ast::attr(astv).where, code_range_);

  if (ast::attr(astv).to_call)
    throw error("An integer constant is not to be called", ast::attr(astv).where, code_range_);

  return new value(llvm::ConstantInt::get(builder_.getInt32Ty(), ast::val(astv)), astv);
}

value* translator::operator()(ast::boolean astv)
{
  if (ast::attr(astv).lval)
    throw error("A boolean constant is not to be assigned", ast::attr(astv).where, code_range_);

  if (ast::attr(astv).to_call)
    throw error("A boolean constant is not to be called", ast::attr(astv).where, code_range_);

  return new value(llvm::ConstantInt::get(builder_.getInt1Ty(), ast::val(astv)), astv);
}

value* translator::operator()(ast::string const& astv)
{
  if (ast::attr(astv).lval)
    throw error("A string constant is not to be assigned", ast::attr(astv).where, code_range_);

  if (ast::attr(astv).to_call)
    throw error("A string constant is not to be called", ast::attr(astv).where, code_range_);

  return new value(builder_.CreateGlobalStringPtr(ast::val(astv)), astv);
}

value* translator::operator()(ast::variable const& astv)
{
  if (auto fp = module_->getFunction(ast::val(astv)))
    return new value(fp, astv);
  try {
    auto vp = thisScope_->symbols().at(ast::val(astv));
    if (ast::attr(astv).lval || vp->isLazy() || !vp->isFundamental())
      return vp;
    else
      return vp->copyWithNewLLVMValue(builder_.CreateLoad(vp->getLLVM()));
  } catch (std::out_of_range&) {
    if (ast::attr(astv).lval) {
      // not found in symbols & to be assigned -> declaration
      return new value(nullptr, astv);
    } else {
      throw error("\"" + ast::val(astv) + "\" has not declared in this scope",
                  ast::attr(astv).where, code_range_);
    }
  }
  assert(false);
}

value* translator::operator()(ast::identifier const& astv)
{
  return new value();  // void
}

value* translator::operator()(ast::array const& astv)
{
  if (ast::attr(astv).lval)
    throw error("An array constant is not to be assigned", ast::attr(astv).where, code_range_);

  if (ast::attr(astv).to_call)
    throw error("An array constant is not to be called", ast::attr(astv).where, code_range_);

  auto firstelem = boost::apply_visitor(*this, ast::val(astv)[0]);
  auto t         = ast::val(astv).empty() ? builder_.getVoidTy() : firstelem->getLLVM()->getType();

  auto aryType = llvm::ArrayType::get(t, ast::val(astv).size());
  auto aryPtr  = builder_.CreateAlloca(aryType);  // Allocate necessary memory
  auto destval = new value(aryPtr, astv);

  std::vector<llvm::Value*> values;
  for (auto const x : ast::val(astv) | boost::adaptors::indexed()) {
    auto v = x.index() == 0 ? firstelem : boost::apply_visitor(*this, x.value());
    if (v->getLLVM()->getType() != t) {
      throw error("all elements of array must have the same type", ast::attr(astv).where,
                  code_range_);
    }
    v->setParent(destval);
    destval->symbols()[std::to_string(x.index())] =
        v;  // store value into fields list so that we can get this value later
    if (!v->isLazy())
      values.push_back(v->getLLVM());
  }

  for (auto const v : values | boost::adaptors::indexed()) {
    std::vector<llvm::Value*> idxList = {builder_.getInt32(0),
                                         builder_.getInt32(static_cast<uint32_t>(v.index()))};
    auto p = builder_.CreateInBoundsGEP(aryType, aryPtr, llvm::ArrayRef<llvm::Value*>(idxList));

    destval->symbols()[std::to_string(v.index())]->setLLVM(p);
    // not good way...? (many to_string)
    builder_.CreateStore(v.value(), p);
  }
  return destval;
}

value* translator::operator()(ast::arglist const& astv)
{
  return new value();  // void
}

value* translator::operator()(ast::structure const& astv)
{
  std::vector<value*> vals;
  std::vector<llvm::Type*> fields;
  auto destv = new value(nullptr, astv);

  for (auto const& m : ast::val(astv) | boost::adaptors::indexed()) {
    auto vp                                     = boost::apply_visitor(*this, m.value().second);
    destv->symbols()[ast::val(m.value().first)] = vp;
    destv->fields()[ast::val(m.value().first)]  = m.index();
    vp->setParent(destv);
    if (!vp->isLazy()) {
      fields.push_back(vp->isFundamental() ? vp->getLLVM()->getType()
                                           : vp->getLLVM()->getType()->getPointerElementType());
      vals.push_back(vp);
    }
  }

  llvm::StructType* structTy = llvm::StructType::create(module_->getContext());
  structTy->setBody(fields);

  auto ptr = builder_.CreateAlloca(structTy);
  destv->setLLVM(ptr);
  for (auto const v : vals | boost::adaptors::indexed()) {
    auto p          = builder_.CreateStructGEP(structTy, ptr, static_cast<uint32_t>(v.index()));
    std::string str = std::find_if(destv->fields().begin(), destv->fields().end(),
                                   [&v](auto& x) { return x.second == v.index(); })
                          ->first;

    if (v.value()->isFundamental()) {
      builder_.CreateStore(v.value()->getLLVM(), p);
    } else {
      std::vector<llvm::Type*> list;
      list.push_back(builder_.getInt8Ty()->getPointerTo());
      list.push_back(builder_.getInt8Ty()->getPointerTo());
      list.push_back(builder_.getInt64Ty());
      list.push_back(builder_.getInt32Ty());
      list.push_back(builder_.getInt1Ty());
      llvm::Function* fmemcpy =
          llvm::Intrinsic::getDeclaration(module_.get(), llvm::Intrinsic::memcpy, list);

      std::vector<llvm::Value*> idxList = {builder_.getInt32(1)};
      auto sizelp                       = builder_.CreatePtrToInt(
          builder_.CreateGEP(v.value()->getLLVM()->getType()->getPointerElementType(),
                             llvm::ConstantPointerNull::get(
                                 llvm::cast<llvm::PointerType>(v.value()->getLLVM()->getType())),
                             idxList),
          builder_.getInt64Ty());  // ptrtoint %A* getelementptr (%A, %A* null, i32 1) to i64

      std::vector<llvm::Value*> arg_values;
      arg_values.push_back(builder_.CreatePointerCast(p, builder_.getInt8PtrTy()));
      arg_values.push_back(
          builder_.CreatePointerCast(v.value()->getLLVM(), builder_.getInt8PtrTy()));
      arg_values.push_back(sizelp);
      arg_values.push_back(builder_.getInt32(0));
      arg_values.push_back(builder_.getInt1(0));
      builder_.CreateCall(fmemcpy, llvm::ArrayRef<llvm::Value*>(arg_values));
    }

    destv->symbols()[str]->setLLVM(p);  // not good for performance
  }

  return destv;
}

value* translator::operator()(ast::function const& fcv)
{
  if (ast::attr(fcv).lval)
    throw error("A function constant is not to be assigned", ast::attr(fcv).where, code_range_);

  auto& args = ast::val(fcv).first;

  llvm::FunctionType* func_type = llvm::FunctionType::get(
      builder_.getVoidTy(), std::vector<llvm::Type*>(args.size(), builder_.getInt32Ty()), false);
  llvm::Function* func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage);

  auto destv = new value(func, fcv, true);  // is_lazy = true

  // maybe useless
  // for (auto const arg : args | boost::adaptors::indexed()) {
  //   destv->symbols()[ast::val(arg.value())] = nullptr
  // }

  return destv;
}

value* translator::operator()(ast::scope const& scv)
{
  if (ast::attr(scv).lval)
    throw error("A scope constant is not to be assigned", ast::attr(scv).where, code_range_);

  auto bb = llvm::BasicBlock::Create(module_->getContext());  // empty

  auto destv = new value(bb, scv, true);  // is_lazy = true
  destv->symbols().insert(thisScope_->symbols().begin(), thisScope_->symbols().end());

  return destv;
}

template <>
value* translator::operator()(ast::op<ast::call, 2> const& op)
{
  std::vector<value*> args;
  std::transform(ast::val(op).begin(), ast::val(op).end(), std::back_inserter(args),
                 [this](auto& o) { return boost::apply_visitor(*this, o); });
  return apply_op(op, args);
}
template <>
value* translator::operator()(ast::op<ast::assign, 2> const& op)
{
  std::vector<value*> args;
  std::transform(ast::val(op).begin(), ast::val(op).end(), std::back_inserter(args),
                 [this](auto& o) { return boost::apply_visitor(*this, o); });
  return apply_op(op, args);
}
template <>
value* translator::operator()(ast::op<ast::dot, 2> const& op)
{
  std::vector<value*> args;
  std::transform(ast::val(op).begin(), ast::val(op).end(), std::back_inserter(args),
                 [this](auto& o) { return boost::apply_visitor(*this, o); });
  return apply_op(op, args);
}
template <>
value* translator::operator()(ast::op<ast::cond, 3> const& op)
{
  std::vector<value*> args;
  std::transform(ast::val(op).begin(), ast::val(op).end(), std::back_inserter(args),
                 [this](auto& o) { return boost::apply_visitor(*this, o); });
  return apply_op(op, args);
}

};  // namespace assembly
};  // namespace scopion
