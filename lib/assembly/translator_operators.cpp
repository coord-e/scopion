#include "scopion/assembly/translator.hpp"
#include "scopion/assembly/value.hpp"

#include "scopion/error.hpp"

#include <boost/range/adaptor/indexed.hpp>

#include <algorithm>
#include <iostream>

namespace scopion
{
namespace assembly
{
value_t translator::apply_op(ast::binary_op<ast::add> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateAdd(get_v(lhs), get_v(rhs));
}

value_t translator::apply_op(ast::binary_op<ast::sub> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateSub(get_v(lhs), get_v(rhs));
}

value_t translator::apply_op(ast::binary_op<ast::mul> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateMul(get_v(lhs), get_v(rhs));
}

value_t translator::apply_op(ast::binary_op<ast::div> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateSDiv(get_v(lhs), get_v(rhs));
}

value_t translator::apply_op(ast::binary_op<ast::rem> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateSRem(get_v(lhs), get_v(rhs));
}

value_t translator::apply_op(ast::binary_op<ast::shl> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateShl(get_v(lhs), get_v(rhs));
}

value_t translator::apply_op(ast::binary_op<ast::shr> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateLShr(get_v(lhs), get_v(rhs));
}

value_t translator::apply_op(ast::binary_op<ast::iand> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateAnd(get_v(lhs), get_v(rhs));
}

value_t translator::apply_op(ast::binary_op<ast::ior> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateOr(get_v(lhs), get_v(rhs));
}

value_t translator::apply_op(ast::binary_op<ast::ixor> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateXor(get_v(lhs), get_v(rhs));
}

value_t translator::apply_op(ast::binary_op<ast::land> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateAnd(
      builder_.CreateICmpNE(get_v(lhs),
                            llvm::Constant::getNullValue(builder_.getInt1Ty())),
      builder_.CreateICmpNE(
          get_v(rhs), llvm::Constant::getNullValue(builder_.getInt1Ty())));
}

value_t translator::apply_op(ast::binary_op<ast::lor> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateOr(
      builder_.CreateICmpNE(get_v(lhs),
                            llvm::Constant::getNullValue(builder_.getInt1Ty())),
      builder_.CreateICmpNE(
          get_v(rhs), llvm::Constant::getNullValue(builder_.getInt1Ty())));
}

value_t translator::apply_op(ast::binary_op<ast::eeq> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateICmpEQ(get_v(lhs), get_v(rhs));
}

value_t translator::apply_op(ast::binary_op<ast::neq> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateICmpNE(get_v(lhs), get_v(rhs));
}

value_t translator::apply_op(ast::binary_op<ast::gt> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateICmpSGT(get_v(lhs), get_v(rhs));
}

value_t translator::apply_op(ast::binary_op<ast::lt> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateICmpSLT(get_v(lhs), get_v(rhs));
}

value_t translator::apply_op(ast::binary_op<ast::gtq> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateICmpSGE(get_v(lhs), get_v(rhs));
}

value_t translator::apply_op(ast::binary_op<ast::ltq> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  return builder_.CreateICmpSLE(get_v(lhs), get_v(rhs));
}

value_t translator::apply_op(ast::binary_op<ast::assign> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  if (rhs.type() != typeid(llvm::Value*)) {  // If rhs is lazy value
    auto lvar = ast::unpack<ast::variable>(
        ast::val(op)[0]);  // auto6 lvar = not working
    getSymbols(currentScope_)[ast::val(lvar)] = rhs;  // just aliasing
    return rhs;
  }

  auto lval = get_v(lhs);
  auto rval = get_v(rhs);
  if (!lval) {  // first appear in the block (variable declaration)
    auto lvar = ast::unpack<ast::variable>(
        ast::val(op)[0]);  // auto6 lvar = not working
    if (rval->getType()->isVoidTy())
      throw error("Cannot assign the value of void type", ast::attr(op).where,
                  code_range_);
    auto lptr = builder_.CreateAlloca(rval->getType(), nullptr, ast::val(lvar));
    builder_.CreateStore(rval, lptr);
    getSymbols(currentScope_)[ast::val(lvar)] = lptr;
    return builder_.CreateLoad(lptr);
  } else {  // assigning
    if (lval->getType()->isPointerTy()) {
      if (lval->getType()->getPointerElementType() == rval->getType()) {
        builder_.CreateStore(rval, lval);
      } else {
        throw error(
            "Cannot assign to different type of value (assigning " +
                getNameString(rval->getType()) + " into " +
                getNameString(lval->getType()->getPointerElementType()) + ")",
            ast::attr(op).where, code_range_);
      }
    } else {
      throw error("Cannot assign to non-pointer value (" +
                      getNameString(lval->getType()) + ")",
                  ast::attr(op).where, code_range_);
    }
    return builder_.CreateLoad(lval);
  }
}

value_t translator::apply_op(ast::binary_op<ast::call> const& op,
                             value_t lhs,
                             value_t const rhs)
{
  if (lhs.type() == typeid(llvm::Value*)) {
    auto lval = get_v(lhs);
    if (!lval->getType()->isPointerTy()) {
      throw error("Cannot call a non-pointer value", ast::attr(op).where,
                  code_range_);
    } else if (!lval->getType()->getPointerElementType()->isFunctionTy()) {
      throw error("Cannot call a value which is not a function pointer",
                  ast::attr(op).where, code_range_);
    } else {
      auto arglist = ast::unpack<ast::arglist>(ast::val(op)[1]);

      std::vector<llvm::Value*> arg_values;

      std::transform(ast::val(arglist).begin(), ast::val(arglist).end(),
                     std::back_inserter(arg_values), [this](auto& x) {
                       return get_v(boost::apply_visitor(*this, x));
                     });
      return builder_.CreateCall(lval,
                                 llvm::ArrayRef<llvm::Value*>(arg_values));
    }
  }
  if (lhs.type() == typeid(lazy_value<llvm::Function>))
    return apply_lazy(boost::get<lazy_value<llvm::Function>>(lhs), op);
  if (lhs.type() == typeid(lazy_value<llvm::BasicBlock>))
    return apply_lazy(boost::get<lazy_value<llvm::BasicBlock>>(lhs), op);

  assert(false);
  return nullptr;
}

value_t translator::apply_op(ast::binary_op<ast::at> const& op,
                             value_t const lhs,
                             value_t const rhs)
{
  auto r    = get_v(rhs);
  auto rval = r->getType()->isPointerTy() ? builder_.CreateLoad(r) : r;

  auto lval = get_v(lhs);

  if (!rval->getType()->isIntegerTy()) {
    throw error(
        "Array's index must be integer, not " + getNameString(rval->getType()),
        ast::attr(op).where, code_range_);
  }
  if (!lval->getType()->isPointerTy()) {
    throw error("Cannot get element from non-pointer type " +
                    getNameString(lval->getType()),
                ast::attr(op).where, code_range_);
  }

  if (!lval->getType()->getPointerElementType()->isArrayTy()) {
    lval = builder_.CreateLoad(get_v(lval));

    if (!lval->getType()->getPointerElementType()->isArrayTy()) {
      throw error("Cannot get element from non-array type " +
                      getNameString(lval->getType()),
                  ast::attr(op).where, code_range_);
    }
  }
  // Now lval's type is pointer to array

  std::vector<llvm::Value*> idxList = {builder_.getInt32(0), rval};
  auto ep =
      builder_.CreateInBoundsGEP(lval->getType()->getPointerElementType(), lval,
                                 llvm::ArrayRef<llvm::Value*>(idxList));
  if (ast::attr(op).lval)
    return ep;
  else
    return builder_.CreateLoad(ep);
}

value_t translator::apply_op(ast::binary_op<ast::dot> const& op,
                             value_t lhs,
                             value_t const rhs)
{
  auto lval = get_v(lhs);

  auto id = ast::val(ast::unpack<ast::identifier>(ast::val(op)[0]));

  if (!lval->getType()->isPointerTy())
    throw error("Cannot get \"" + id + "\" from non-pointer type " +
                    getNameString(lval->getType()),
                ast::attr(op).where, code_range_);

  lval = lval->getType()->getPointerElementType()->isPointerTy()
             ? builder_.CreateLoad(lval)
             : lval;

  auto typeString = getNameString(lval->getType());

  if (!lval->getType()->getPointerElementType()->isStructTy())
    throw error(
        "Cannot get \"" + id + "\" from non-structure type " + typeString,
        ast::attr(op).where, code_range_);

  std::string structName;
  std::copy(typeString.begin() + 1, typeString.end() - 1,
            std::back_inserter(structName));  // %TYPENAME* -> TYPENAME
  auto& themap = fields_map[structName];

  auto it = std::find(themap.cbegin(), themap.cend(), id);
  if (it == themap.end()) {
    auto structContentStr = getNameString(module_->getTypeByName(structName));
    throw error("No member named \"" + id + "\" in the structure of" +
                    structContentStr.erase(0, structContentStr.find("=") + 1),
                ast::attr(op).where,
                code_range_);  // %TYPENAME = type {...} -> type {...}
  }

  auto ptr = builder_.CreateStructGEP(
      lval->getType()->getPointerElementType(), lval,
      static_cast<uint32_t>(std::distance(themap.cbegin(), it)));

  if (ast::attr(op).lval)
    return ptr;
  else
    return builder_.CreateLoad(ptr);
}

value_t translator::apply_op(ast::single_op<ast::load> const& op,
                             value_t const value)
{
  auto vv = get_v(value);
  if (!vv->getType()->isPointerTy())
    throw error(
        "Cannot load from non-pointer type " + getNameString(vv->getType()),
        ast::attr(op).where, code_range_);
  return builder_.CreateLoad(vv);
}

value_t translator::apply_op(ast::single_op<ast::ret> const& op,
                             value_t const value)
{
  return builder_.CreateRet(get_v(value));
}

value_t translator::apply_op(ast::single_op<ast::lnot> const& op,
                             value_t const value)
{
  return builder_.CreateXor(
      builder_.CreateICmpNE(get_v(value),
                            llvm::Constant::getNullValue(builder_.getInt1Ty())),
      builder_.getInt32(1));
}

value_t translator::apply_op(ast::single_op<ast::inot> const& op,
                             value_t const value)
{
  return builder_.CreateXor(get_v(value), builder_.getInt32(1));
}

value_t translator::apply_op(ast::single_op<ast::inc> const& op,
                             value_t const value)
{
  return builder_.CreateAdd(get_v(value), builder_.getInt32(1));
}

value_t translator::apply_op(ast::single_op<ast::dec> const& op,
                             value_t const value)
{
  return builder_.CreateSub(get_v(value), builder_.getInt32(1));
}

value_t translator::apply_op(ast::ternary_op<ast::cond> const& op,
                             value_t const first,
                             value_t const second,
                             value_t const third)
{
  if (second.type() != typeid(lazy_value<llvm::BasicBlock>) ||
      third.type() != typeid(lazy_value<llvm::BasicBlock>))
    throw error("The arguments of ternary operator should be a block",
                ast::attr(op).where, code_range_);

  auto secondv = boost::get<lazy_value<llvm::BasicBlock>>(second);
  auto thirdv  = boost::get<lazy_value<llvm::BasicBlock>>(third);

  llvm::BasicBlock* thenbb =
      llvm::BasicBlock::Create(module_->getContext(), createNewBBName(),
                               builder_.GetInsertBlock()->getParent());
  llvm::BasicBlock* elsebb =
      llvm::BasicBlock::Create(module_->getContext(), createNewBBName(),
                               builder_.GetInsertBlock()->getParent());

  auto pb = builder_.GetInsertBlock();
  auto pp = builder_.GetInsertPoint();

  llvm::BasicBlock* mergebb =
      llvm::BasicBlock::Create(module_->getContext(), createNewBBName(),
                               builder_.GetInsertBlock()->getParent());

  bool mergebbShouldBeErased = true;

  auto prevScope = currentScope_;

  builder_.SetInsertPoint(thenbb);
  secondv.setValue(thenbb);
  currentScope_ = secondv;
  if (apply_bb(secondv)) {
    builder_.CreateBr(mergebb);
    mergebbShouldBeErased &= false;
  }

  builder_.SetInsertPoint(elsebb);
  thirdv.setValue(elsebb);
  currentScope_ = thirdv;
  if (apply_bb(thirdv)) {
    builder_.CreateBr(mergebb);
    mergebbShouldBeErased &= false;
  }

  currentScope_ = prevScope;

  if (mergebbShouldBeErased)
    mergebb->eraseFromParent();

  builder_.SetInsertPoint(pb, pp);

  builder_.CreateCondBr(get_v(first), thenbb, elsebb);

  if (!mergebbShouldBeErased)
    builder_.SetInsertPoint(mergebb);

  return value_t();
}

template <typename T>
llvm::Value* translator::apply_lazy(lazy_value<llvm::BasicBlock> value,
                                    ast::value_wrapper<T> const& astv)
{
  auto pb = builder_.GetInsertBlock();
  auto pp = builder_.GetInsertPoint();

  auto nb = llvm::BasicBlock::Create(module_->getContext(), createNewBBName(),
                                     builder_.GetInsertBlock()->getParent());

  auto theblock =
      llvm::BasicBlock::Create(module_->getContext(), createNewBBName(),
                               builder_.GetInsertBlock()->getParent());
  builder_.SetInsertPoint(theblock);
  value.setValue(theblock);
  auto prevScope = currentScope_;

  currentScope_ = value;
  bool non_rb   = apply_bb(value);
  if (non_rb)
    builder_.CreateBr(nb);
  else
    nb->eraseFromParent();

  currentScope_ = prevScope;

  builder_.SetInsertPoint(pb, pp);

  builder_.CreateBr(theblock);

  if (non_rb)
    builder_.SetInsertPoint(nb);

  return nullptr;
}

template <typename T>
llvm::Value* translator::apply_lazy(lazy_value<llvm::Function> value,
                                    ast::value_wrapper<T> const& astv)
{
  auto arglist = ast::unpack<ast::arglist>(ast::val(astv)[1]);
  // auto& arglist = not working eh?
  if (value.getValue()
          ->getType()
          ->getPointerElementType()
          ->getFunctionNumParams() != ast::val(arglist).size())
    throw error("The number of arguments doesn't match: required " +
                    std::to_string(value.getValue()
                                       ->getType()
                                       ->getPointerElementType()
                                       ->getFunctionNumParams()) +
                    " but supplied " + std::to_string(ast::val(arglist).size()),
                ast::attr(astv).where, code_range_);

  assert(value.symbols.size() == ast::val(arglist).size());
  std::vector<std::string> arg_names;
  for (auto const arg : value.symbols | boost::adaptors::indexed()) {
    std::string::const_iterator it;
    for (it = arg.value().first.cbegin(); *it != ':'; it++) {
    }
    arg_names.push_back(std::string(std::next(it), arg.value().first.cend()));
  }

  std::vector<value_t> arg_values;
  std::vector<llvm::Type*> arg_types;
  std::vector<llvm::Type*> arg_types_for_func;

  std::transform(ast::val(arglist).begin(), ast::val(arglist).end(),
                 std::back_inserter(arg_values),
                 [this](auto& x) { return boost::apply_visitor(*this, x); });

  for (auto const& arg_value : arg_values) {
    auto type = get_v(arg_value)->getType();
    if (arg_value.type() == typeid(llvm::Value*))
      arg_types_for_func.push_back(type);
    arg_types.push_back(type);
  }

  llvm::FunctionType* func_type =
      llvm::FunctionType::get(builder_.getVoidTy(), arg_types_for_func, false);
  llvm::Function* func =
      llvm::Function::Create(func_type, llvm::Function::ExternalLinkage,
                             value->getName(), module_.get());
  llvm::BasicBlock* entry =
      llvm::BasicBlock::Create(module_->getContext(),
                               "entry_survey",  // BasicBlockの名前
                               func);
  auto prevScope = currentScope_;
  currentScope_  = lazy_value<llvm::BasicBlock>(entry, value.getInsts());

  auto pb = builder_.GetInsertBlock();
  auto pp = builder_.GetInsertPoint();

  builder_.SetInsertPoint(entry);  // entryの開始

  getSymbols(currentScope_)["self"] =
      builder_.CreateAlloca(func->getType(), nullptr, "self");

  for (auto const& arg_name : arg_names | boost::adaptors::indexed()) {
    if (arg_values[arg_name.index()].type() == typeid(llvm::Value*)) {
      getSymbols(currentScope_)[arg_name.value()] =
          builder_.CreateAlloca(arg_types[arg_name.index()], nullptr,
                                arg_name.value());  // declare arguments
    } else {
      getSymbols(currentScope_)[arg_name.value()] =
          arg_values[arg_name.index()];
    }
  }

  for (auto const& line : value.getInsts()) {
    auto e = ast::set_survey(line, true);
    boost::apply_visitor(*this, e);
  }

  llvm::Type* ret_type = nullptr;
  for (auto const& bb : *func) {
    for (auto itr = bb.getInstList().begin(); itr != bb.getInstList().end();
         ++itr) {
      if ((*itr).getOpcode() == llvm::Instruction::Ret) {
        if ((*itr).getOperand(0)->getType() != ret_type) {
          if (ret_type == nullptr) {
            ret_type = (*itr).getOperand(0)->getType();
          } else {
            throw error("All return values must have the same type",
                        ast::attr(astv).where, code_range_);
          }
        }
      }
    }
  }

  if (!ret_type) {
    builder_.CreateRetVoid();
    ret_type = builder_.getVoidTy();
  }

  func->eraseFromParent();  // remove old one

  llvm::Function* newfunc;
  if (ast::attr(astv).survey) {
    newfunc = llvm::Function::Create(
        llvm::FunctionType::get(ret_type, arg_types_for_func, false),
        llvm::Function::ExternalLinkage);
  } else {  // Create the real content of function if it
            // isn't in survey

    newfunc = llvm::Function::Create(
        llvm::FunctionType::get(ret_type, arg_types_for_func, false),
        llvm::Function::ExternalLinkage, value->getName(), module_.get());

    llvm::BasicBlock* newentry =
        llvm::BasicBlock::Create(module_->getContext(),
                                 "entry",  // BasicBlockの名前
                                 newfunc);

    currentScope_ = lazy_value<llvm::BasicBlock>(newentry, value.getInsts());
    builder_.SetInsertPoint(newentry);

    auto selfptr = builder_.CreateAlloca(newfunc->getType(), nullptr, "self");
    getSymbols(currentScope_)["self"] = selfptr;
    builder_.CreateStore(newfunc, selfptr);

    auto it = newfunc->getArgumentList().begin();
    for (auto const& arg_name : arg_names | boost::adaptors::indexed()) {
      auto argv = arg_values[arg_name.index()];
      if (argv.type() == typeid(llvm::Value*)) {
        auto aptr =
            builder_.CreateAlloca(arg_types[arg_name.index()], nullptr,
                                  arg_name.value());  // declare arguments
        getSymbols(currentScope_)[arg_name.value()] = aptr;
        builder_.CreateStore(&(*it), aptr);
        it++;
      } else {
        std::cout << arg_name.value() << " is lazy" << std::endl;
        getSymbols(currentScope_)[arg_name.value()] = argv;
      }
    }

    for (auto const& line : value.getInsts()) {
      boost::apply_visitor(*this, line);
    }

    if (ret_type->isVoidTy()) {
      builder_.CreateRetVoid();
    }
  }

  currentScope_ = prevScope;
  builder_.SetInsertPoint(pb, pp);  // entryを抜ける

  // value.getValue()->eraseFromParent();

  std::vector<llvm::Value*> arg_llvm_values;

  for (auto const& arg_value : arg_values) {
    if (arg_value.type() == typeid(llvm::Value*))
      arg_llvm_values.push_back(get_v(arg_value));
  }

  return builder_.CreateCall(newfunc,
                             llvm::ArrayRef<llvm::Value*>(arg_llvm_values));
}

};  // namespace assembly
};  // namespace scopion
