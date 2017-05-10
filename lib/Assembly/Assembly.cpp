#include "Assembly/Assembly.h"
#include <algorithm>
#include <boost/range/adaptor/indexed.hpp>
#include <iostream>

namespace scopion {
assembly::assembly(std::string const &name)
    : boost::static_visitor<llvm::Value *>(),
      module_(new llvm::Module(name, context_)), builder_(context_) {
  {
    std::vector<llvm::Type *> args = {builder_.getInt8Ty()->getPointerTo()};

    variables_["printf"] = module_->getOrInsertFunction(
        "printf",
        llvm::FunctionType::get(builder_.getInt32Ty(),
                                llvm::ArrayRef<llvm::Type *>(args), true));
    variables_["puts"] = module_->getOrInsertFunction(
        "puts",
        llvm::FunctionType::get(builder_.getInt32Ty(),
                                llvm::ArrayRef<llvm::Type *>(args), true));
  }
  {
    std::vector<llvm::Type *> func_args_type;
    func_args_type.push_back(builder_.getInt32Ty());

    llvm::FunctionType *llvm_func_type = llvm::FunctionType::get(
        builder_.getInt32Ty(), func_args_type, /*可変長引数=*/false);
    llvm::Function *llvm_func =
        llvm::Function::Create(llvm_func_type, llvm::Function::ExternalLinkage,
                               "printn", module_.get());

    llvm::BasicBlock *entry =
        llvm::BasicBlock::Create(context_,
                                 "entry_printn", // BasicBlockの名前
                                 llvm_func);
    builder_.SetInsertPoint(entry); // entryの開始
    std::vector<llvm::Value *> args = {
        builder_.CreateGlobalStringPtr("%d\n"),
        &(*(llvm_func->getArgumentList().begin()))};
    builder_.CreateCall(variables_["printf"], args);
    builder_.CreateRet(llvm::ConstantInt::get(builder_.getInt32Ty(), 0, true));

    variables_["printn"] = llvm_func;
  }
} // namespace scopion

llvm::Value *assembly::operator()(ast::value value) {
  switch (value.which()) {
  case 0: // int
    return llvm::ConstantInt::get(builder_.getInt32Ty(),
                                  boost::get<int>(value));
  case 1: // bool
    return llvm::ConstantInt::get(builder_.getInt1Ty(),
                                  boost::get<bool>(value));
  case 2: // string
    return builder_.CreateGlobalStringPtr(boost::get<std::string>(value));
  case 3: // variable
  {
    auto &v = boost::get<ast::variable>(value);
    if (v.isFunc) {
      if (variables_.count(v.name) == 0)
        throw std::runtime_error("Function \"" + v.name +
                                 "\" has not declared");
      return nullptr;
    } else {
      if (v.rl) { // true - rval. load and return
        if (variables_.count(v.name) == 0)
          throw std::runtime_error("\"" + v.name + "\" has not declared");
        return builder_.CreateLoad(variables_[v.name], v.name);
      } else // false - lval to declare. nothing to return
        return nullptr;
    }
  }
  case 4: // array
  {
    auto &ary = boost::get<ast::array>(value).elements;
    std::vector<llvm::Value *> values;
    for (auto const &elm : ary) {
      // Convert exprs to llvm::Value* and store it into new vector
      values.push_back(boost::apply_visitor(*this, elm));
    }

    auto t = values[0]->getType();
    // check if the type of all elements of array is same
    if (!std::all_of(values.begin(), values.end(),
                     [&t](auto v) { return t == v->getType(); }))
      throw std::runtime_error("all elements of array must have the same type");

    auto aryType = llvm::ArrayType::get(t, ary.size());
    auto aryPtr = builder_.CreateAlloca(aryType); // Allocate necessary memory
    for (auto const &v : values | boost::adaptors::indexed()) {
      std::vector<llvm::Value *> idxList = {builder_.getInt32(0),
                                            builder_.getInt32(v.index())};
      auto p = builder_.CreateInBoundsGEP(
          aryType, aryPtr, llvm::ArrayRef<llvm::Value *>(idxList));

      builder_.CreateStore(v.value(), p);
    }
    return aryPtr;
  }
  default:
    assert(false);
  }
} // namespace scopion

void assembly::IRGen(std::vector<ast::expr> const &asts) {
  auto *main_func = llvm::Function::Create(
      llvm::FunctionType::get(builder_.getInt32Ty(), false),
      llvm::Function::ExternalLinkage, "main", module_.get());

  builder_.SetInsertPoint(
      llvm::BasicBlock::Create(context_, "entry", main_func));

  for (auto const &i : asts) {
    /*std::vector<llvm::Value *> args = {builder_.CreateGlobalStringPtr("%d\n"),
                                       boost::apply_visitor(*this, i)};
    builder_.CreateCall(variables_["printf"],
                        llvm::ArrayRef<llvm::Value *>(args));*/
    boost::apply_visitor(*this, i);
  }

  builder_.CreateRet(llvm::ConstantInt::get(builder_.getInt32Ty(), 0));
}

std::string assembly::getIR() {
  std::string result;
  llvm::raw_string_ostream stream(result);

  module_->print(stream, nullptr);
  return result;
}

std::string assembly::getTypeStr(llvm::Type *t) {
  std::string type_string;
  llvm::raw_string_ostream stream(type_string);
  t->print(stream);
  return stream.str();
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::add> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateAdd(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::sub> const &,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateSub(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::mul> const &,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateMul(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::div> const &,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateSDiv(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::rem> const &,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateSRem(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::shl> const &,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateShl(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::shr> const &,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateLShr(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::iand> const &,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateAnd(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::ior> const &,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateOr(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::ixor> const &,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateXor(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::land> const &,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateAnd(
      builder_.CreateICmpNE(lhs,
                            llvm::Constant::getNullValue(builder_.getInt1Ty())),
      builder_.CreateICmpNE(
          rhs, llvm::Constant::getNullValue(builder_.getInt1Ty())));
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::lor> const &,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateOr(
      builder_.CreateICmpNE(lhs,
                            llvm::Constant::getNullValue(builder_.getInt1Ty())),
      builder_.CreateICmpNE(
          rhs, llvm::Constant::getNullValue(builder_.getInt1Ty())));
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::eeq> const &,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpEQ(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::neq> const &,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpNE(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::gt> const &,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpSGT(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::lt> const &,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpSLT(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::gtq> const &,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpSGE(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::ltq> const &,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpSLE(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::assign> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  auto &&lvar = boost::get<ast::variable>(boost::get<ast::value>(op.lhs));
  if (rhs->getType()->isPointerTy()) {
    variables_[lvar.name] = rhs;
  } else {
    auto var_pointer =
        builder_.CreateAlloca(rhs->getType(), nullptr, lvar.name);
    builder_.CreateStore(rhs, var_pointer);
    variables_[lvar.name] = var_pointer;
  }
  return rhs;
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::call> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  auto &&lvar = boost::get<ast::variable>(boost::get<ast::value>(op.lhs));
  std::vector<llvm::Value *> args = {rhs};
  return builder_.CreateCall(variables_[lvar.name],
                             llvm::ArrayRef<llvm::Value *>(args));
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::at> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  if (!rhs->getType()->isIntegerTy()) {
    throw std::runtime_error("Array's index must be integer, not " +
                             getTypeStr(rhs->getType()));
  }

  if (!lhs->getType()->isPointerTy()) {
    throw std::runtime_error("You cannot get element from non-pointer type " +
                             getTypeStr(lhs->getType()));
  }

  if (!lhs->getType()->getPointerElementType()->isArrayTy()) {
    throw std::runtime_error(
        "You cannot get element from non-array pointer type " +
        getTypeStr(lhs->getType()->getPointerElementType()));
  }

  std::vector<llvm::Value *> idxList = {builder_.getInt32(0), rhs};
  return builder_.CreateInBoundsGEP(lhs->getType()->getPointerElementType(),
                                    lhs,
                                    llvm::ArrayRef<llvm::Value *>(idxList));
}
}; // namespace scopion
