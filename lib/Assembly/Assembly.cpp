#include "Assembly/Assembly.h"
#include <algorithm>
#include <boost/range/adaptor/indexed.hpp>
#include <iostream>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/Support/raw_ostream.h>

namespace scopion {
assembly::assembly(std::string const &name)
    : boost::static_visitor<llvm::Value *>(),
      module_(new llvm::Module(name, context_)), builder_(context_) {
  {
    std::vector<llvm::Type *> args = {builder_.getInt8Ty()->getPointerTo()};

    module_->getOrInsertFunction(
        "printf",
        llvm::FunctionType::get(builder_.getInt32Ty(),
                                llvm::ArrayRef<llvm::Type *>(args), true));
    module_->getOrInsertFunction(
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
    builder_.CreateCall(module_->getFunction("printf"), args);
    builder_.CreateRet(llvm::ConstantInt::get(builder_.getInt32Ty(), 0, true));
  }
}

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
      auto *valp = module_->getFunction(v.name);
      if (valp != nullptr) {
        return valp;
      } else {
        auto *varp =
            builder_.GetInsertBlock()->getValueSymbolTable()->lookup(v.name);
        if (varp != nullptr) {
          if (varp->getType()->getPointerElementType()->isPointerTy()) {
            if (varp->getType()
                    ->getPointerElementType()
                    ->getPointerElementType()
                    ->isFunctionTy()) {
              return varp;
            }
          }
          throw std::runtime_error(
              "Variable \"" + v.name + "\" is not a function but " +
              getTypeStr(varp->getType()->getPointerElementType()));
        } else {
          throw std::runtime_error("Function \"" + v.name +
                                   "\" has not declared in this scope");
        }
      }
    } else {
      auto *valp =
          builder_.GetInsertBlock()->getValueSymbolTable()->lookup(v.name);
      if (v.rl) {
        if (valp != nullptr) {
          return valp;
        } else {
          throw std::runtime_error("\"" + v.name + "\" has not declared");
        }
      } else {
        return valp;
      }
    }
    assert(false);
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
  case 5: // function
  {
    auto &lines = boost::get<ast::function>(value).lines;
    // 戻り値の型
    llvm::Type *func_ret_type = builder_.getVoidTy();
    // 引数の型
    std::vector<llvm::Type *> func_args_type;
    func_args_type.push_back(builder_.getInt32Ty());

    llvm::FunctionType *func_type =
        llvm::FunctionType::get(func_ret_type, func_args_type, false);
    llvm::Function *func = llvm::Function::Create(
        func_type, llvm::Function::ExternalLinkage, "", module_.get());
    llvm::BasicBlock *entry =
        llvm::BasicBlock::Create(context_,
                                 "entry", // BasicBlockの名前
                                 func);
    auto *pb = builder_.GetInsertBlock();
    auto pp = builder_.GetInsertPoint();

    builder_.SetInsertPoint(entry); // entryの開始
    for (auto const &line : lines) {
      boost::apply_visitor(*this, line);
    }
    builder_.CreateRetVoid();
    builder_.SetInsertPoint(pb, pp); // entryを抜ける
    return func;
  }
  default:
    assert(false);
  }
} // namespace scopion

void assembly::IRGen(ast::expr const &tree) {
  auto *main_func = llvm::Function::Create(
      llvm::FunctionType::get(builder_.getInt32Ty(), false),
      llvm::Function::ExternalLinkage, "main", module_.get());

  builder_.SetInsertPoint(
      llvm::BasicBlock::Create(context_, "main_entry", main_func));

  std::vector<llvm::Value *> args = {builder_.getInt32(0)};
  builder_.CreateCall(boost::apply_visitor(*this, tree),
                      llvm::ArrayRef<llvm::Value *>(args));

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

llvm::Value *assembly::apply_op(ast::binary_op<ast::sub> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateSub(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::mul> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateMul(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::div> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateSDiv(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::rem> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateSRem(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::shl> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateShl(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::shr> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateLShr(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::iand> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateAnd(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::ior> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateOr(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::ixor> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateXor(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::land> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateAnd(
      builder_.CreateICmpNE(lhs,
                            llvm::Constant::getNullValue(builder_.getInt1Ty())),
      builder_.CreateICmpNE(
          rhs, llvm::Constant::getNullValue(builder_.getInt1Ty())));
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::lor> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateOr(
      builder_.CreateICmpNE(lhs,
                            llvm::Constant::getNullValue(builder_.getInt1Ty())),
      builder_.CreateICmpNE(
          rhs, llvm::Constant::getNullValue(builder_.getInt1Ty())));
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::eeq> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpEQ(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::neq> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpNE(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::gt> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpSGT(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::lt> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpSLT(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::gtq> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpSGE(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::ltq> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpSLE(lhs, rhs);
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::assign> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  if (lhs == nullptr) { // first appear in the block (only variable)
    auto &&lvar = boost::get<ast::variable>(boost::get<ast::value>(op.lhs));
    lhs = builder_.CreateAlloca(rhs->getType(), nullptr, lvar.name);
    builder_.CreateStore(rhs, lhs);
  } else {
    if (lhs->getType()->isPointerTy()) {
      builder_.CreateStore(rhs, lhs);
    } else {
      throw std::runtime_error("Cannot assign to non-pointer value (" +
                               getTypeStr(lhs->getType()) + ")");
    }
  }
  return lhs;
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::call> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  if (!lhs->getType()->isPointerTy())
    throw std::runtime_error("Cannot call function from non-pointer type " +
                             getTypeStr(lhs->getType()));

  if (!lhs->getType()->getPointerElementType()->isFunctionTy())
    throw std::runtime_error(
        "Cannot call function from non-function pointer type " +
        getTypeStr(lhs->getType()));

  std::vector<llvm::Value *> args = {rhs};
  return builder_.CreateCall(lhs, llvm::ArrayRef<llvm::Value *>(args));
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::at> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  auto *rval = rhs->getType()->isPointerTy() ? builder_.CreateLoad(rhs) : rhs;
  if (!rval->getType()->isIntegerTy()) {
    throw std::runtime_error("Array's index must be integer, not " +
                             getTypeStr(rval->getType()));
  }
  if (!lhs->getType()->isPointerTy()) {
    throw std::runtime_error("Cannot get element from non-pointer type " +
                             getTypeStr(lhs->getType()));
  }

  auto *lval = lhs;

  if (!lhs->getType()->getPointerElementType()->isArrayTy()) {
    lval = builder_.CreateLoad(lhs);

    if (!lval->getType()->getPointerElementType()->isArrayTy()) {
      throw std::runtime_error("Cannot get element from non-array type " +
                               getTypeStr(lhs->getType()));
    }
  }
  // Now lval's type is pointer to array

  std::vector<llvm::Value *> idxList = {builder_.getInt32(0), rval};
  return builder_.CreateInBoundsGEP(lval->getType()->getPointerElementType(),
                                    lval,
                                    llvm::ArrayRef<llvm::Value *>(idxList));
}

llvm::Value *assembly::apply_op(ast::binary_op<ast::load> const &op,
                                llvm::Value *lhs, llvm::Value *rhs) {
  if (!lhs->getType()->isPointerTy())
    throw std::runtime_error("Cannot load from non-pointer type " +
                             getTypeStr(lhs->getType()));
  return builder_.CreateLoad(lhs);
}
}; // namespace scopion
