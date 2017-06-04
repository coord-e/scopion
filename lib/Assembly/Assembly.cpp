#include "scopion/assembly/assembly.h"
#include "scopion/exceptions.h"

#include <algorithm>

#include <boost/range/adaptor/indexed.hpp>
#include <boost/range/iterator_range.hpp>

#include <llvm/IR/ValueSymbolTable.h>

namespace scopion {
namespace assembly {

translator::translator(std::unique_ptr<llvm::Module> &&module,
                       llvm::IRBuilder<> const &builder)
    : boost::static_visitor<llvm::Value *>(), module_(std::move(module)),
      builder_(builder) {
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
        llvm::BasicBlock::Create(module_->getContext(),
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

llvm::Value *translator::operator()(parser::parsed const &expr) {
  try {
    auto *val = boost::apply_visitor(*this, expr.ast);
    return val;
  } catch (std::runtime_error e) {
    throw general_error(
        e.what(), expr.ast.where,
        boost::make_iterator_range(expr.code.begin(), expr.code.end()));
  }
}

llvm::Value *translator::operator()(ast::value value) {
  return boost::apply_visitor(*this, value);
}

llvm::Value *translator::operator()(int value) {
  return llvm::ConstantInt::get(builder_.getInt32Ty(), value);
}

llvm::Value *translator::operator()(bool value) {
  return llvm::ConstantInt::get(builder_.getInt1Ty(), value);
}

llvm::Value *translator::operator()(std::string const &value) {
  return builder_.CreateGlobalStringPtr(value);
}

llvm::Value *translator::operator()(ast::variable const &value) {
  if (value.isFunc) {
    auto *valp = module_->getFunction(value.name);
    if (valp != nullptr) {
      return valp;
    } else {
      auto *varp =
          builder_.GetInsertBlock()->getValueSymbolTable()->lookup(value.name);
      if (varp != nullptr) {
        if (varp->getType()->getPointerElementType()->isPointerTy()) {
          if (varp->getType()
                  ->getPointerElementType()
                  ->getPointerElementType()
                  ->isFunctionTy()) {
            return builder_.CreateLoad(varp);
          }
        }
        throw std::runtime_error(
            "Variable \"" + value.name + "\" is not a function but " +
            getNameString(varp->getType()->getPointerElementType()));
      } else {
        throw std::runtime_error("Function \"" + value.name +
                                 "\" has not declared in this scope");
      }
    }
  } else {
    auto *valp =
        builder_.GetInsertBlock()->getValueSymbolTable()->lookup(value.name);
    if (value.lval) {
      return valp;
    } else {
      if (valp != nullptr) {
        return builder_.CreateLoad(valp);
      } else {
        throw std::runtime_error("\"" + value.name + "\" has not declared");
      }
    }
  }
  assert(false);
}
llvm::Value *translator::operator()(ast::array const &value) {
  auto &ary = value.elements;
  std::vector<llvm::Value *> values;
  for (auto const &elm : ary) {
    // Convert exprs to llvm::Value* and store it into new vector
    values.push_back(boost::apply_visitor(*this, elm));
  }

  auto t = values.empty() ? builder_.getVoidTy() : values[0]->getType();
  // check if the type of all elements of array is same
  if (!std::all_of(values.begin(), values.end(),
                   [&t](auto v) { return t == v->getType(); }))
    throw std::runtime_error("all elements of array must have the same type");

  auto aryType = llvm::ArrayType::get(t, ary.size());
  auto aryPtr = builder_.CreateAlloca(aryType); // Allocate necessary memory
  for (auto const &v : values | boost::adaptors::indexed()) {
    std::vector<llvm::Value *> idxList = {builder_.getInt32(0),
                                          builder_.getInt32(v.index())};
    auto p = builder_.CreateInBoundsGEP(aryType, aryPtr,
                                        llvm::ArrayRef<llvm::Value *>(idxList));

    builder_.CreateStore(v.value(), p);
  }
  return aryPtr;
}

llvm::Value *translator::operator()(ast::function const &value) {
  auto &lines = value.lines;

  std::vector<llvm::Type *> func_args_type;
  func_args_type.push_back(builder_.getInt32Ty());
  llvm::FunctionType *func_type =
      llvm::FunctionType::get(builder_.getVoidTy(), func_args_type, false);
  llvm::Function *func = llvm::Function::Create(
      func_type, llvm::Function::ExternalLinkage, "", module_.get());
  llvm::BasicBlock *entry =
      llvm::BasicBlock::Create(module_->getContext(),
                               "entryf", // BasicBlockの名前
                               func);
  auto *pb = builder_.GetInsertBlock();
  auto pp = builder_.GetInsertPoint();

  builder_.SetInsertPoint(entry); // entryの開始

  builder_.CreateAlloca(func->getType(), nullptr, "self");

  if (!func->arg_empty()) { // if there are any arguments
    builder_.CreateAlloca(builder_.getInt32Ty(), nullptr,
                          "arg"); // declare "arg"
  }

  for (auto const &line : lines) {
    boost::apply_visitor(*this, line); // If lines has function definition, the
                                       // function will be defined twice.
  }

  llvm::Type *ret_type = nullptr;
  for (auto itr = entry->getInstList().begin();
       itr != entry->getInstList().end(); ++itr) {
    if ((*itr).getOpcode() == llvm::Instruction::Ret) {
      if ((*itr).getOperand(0)->getType() != ret_type) {
        if (ret_type == nullptr) {
          ret_type = (*itr).getOperand(0)->getType();
        } else {
          throw std::runtime_error("All return values must have the same type");
        }
      }
    }
  }
  if (ret_type == nullptr) {
    builder_.CreateRetVoid();
    ret_type = builder_.getVoidTy();
  }

  func->eraseFromParent(); // remove old one

  std::vector<llvm::Type *> args_type = {builder_.getInt32Ty()};
  llvm::Function *newfunc = llvm::Function::Create(
      llvm::FunctionType::get(ret_type, func_args_type, false),
      llvm::Function::ExternalLinkage, "", module_.get());
  llvm::BasicBlock *newentry =
      llvm::BasicBlock::Create(module_->getContext(),
                               "entry", // BasicBlockの名前
                               newfunc);
  builder_.SetInsertPoint(newentry);

  auto selfptr = builder_.CreateAlloca(newfunc->getType(), nullptr, "self");
  builder_.CreateStore(newfunc, selfptr);
  if (!func->arg_empty()) { // if there are any arguments
    auto aptr = builder_.CreateAlloca(builder_.getInt32Ty(), nullptr,
                                      "arg"); // declare "arg"
    builder_.CreateStore(&(*newfunc->getArgumentList().begin()), aptr);
  }

  for (auto const &line : lines) {
    boost::apply_visitor(*this, line);
  }
  if (ret_type == builder_.getVoidTy()) {
    builder_.CreateRetVoid();
  }

  builder_.SetInsertPoint(pb, pp); // entryを抜ける

  return newfunc;
}

}; // namespace assembly
}; // namespace scopion
