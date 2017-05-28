#include "scopion/assembly/assembly.h"
#include "scopion/exceptions.h"

#include <algorithm>

#include <boost/range/adaptor/indexed.hpp>
#include <boost/range/iterator_range.hpp>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/Support/TargetSelect.h>

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
                               "entry", // BasicBlockの名前
                               func);
  auto *pb = builder_.GetInsertBlock();
  auto pp = builder_.GetInsertPoint();

  builder_.SetInsertPoint(entry); // entryの開始

  llvm::Value *aptr;
  if (!func->arg_empty()) { // if there are any arguments
    aptr = builder_.CreateAlloca(builder_.getInt32Ty(), nullptr,
                                 "arg"); // declare "arg"
  }

  for (auto const &line : lines) {
    boost::apply_visitor(*this, line);
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

  std::vector<llvm::Type *> args_type = {builder_.getInt32Ty()};
  llvm::Function *newfunc = llvm::Function::Create(
      llvm::FunctionType::get(ret_type, func_args_type, false),
      llvm::Function::ExternalLinkage, "", module_.get());

  if (!newfunc->arg_empty()) { // if there are any arguments
    builder_.SetInsertPoint(
        entry,
        ++entry->getFirstInsertionPt()); // To the after of declation of "arg"
    builder_.CreateStore(&(*newfunc->getArgumentList().begin()), aptr);
  }

  newfunc->getBasicBlockList().splice(
      newfunc->begin(),
      func->getBasicBlockList()); // copy to new function
  func->eraseFromParent();        // remove old one

  builder_.SetInsertPoint(pb, pp); // entryを抜ける

  return newfunc;
}

llvm::Value *translator::apply_op(ast::binary_op<ast::add> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateAdd(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::sub> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateSub(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::mul> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateMul(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::div> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateSDiv(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::rem> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateSRem(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::shl> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateShl(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::shr> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateLShr(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::iand> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateAnd(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::ior> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateOr(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::ixor> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateXor(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::land> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateAnd(
      builder_.CreateICmpNE(lhs,
                            llvm::Constant::getNullValue(builder_.getInt1Ty())),
      builder_.CreateICmpNE(
          rhs, llvm::Constant::getNullValue(builder_.getInt1Ty())));
}

llvm::Value *translator::apply_op(ast::binary_op<ast::lor> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateOr(
      builder_.CreateICmpNE(lhs,
                            llvm::Constant::getNullValue(builder_.getInt1Ty())),
      builder_.CreateICmpNE(
          rhs, llvm::Constant::getNullValue(builder_.getInt1Ty())));
}

llvm::Value *translator::apply_op(ast::binary_op<ast::eeq> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpEQ(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::neq> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpNE(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::gt> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpSGT(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::lt> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpSLT(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::gtq> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpSGE(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::ltq> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateICmpSLE(lhs, rhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::assign> const &op,
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
                               getNameString(lhs->getType()) + ")");
    }
  }
  return builder_.CreateLoad(lhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::call> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  if (!lhs->getType()->isPointerTy())
    throw std::runtime_error("Cannot call function from non-pointer type " +
                             getNameString(lhs->getType()));

  if (!lhs->getType()->getPointerElementType()->isFunctionTy())
    throw std::runtime_error(
        "Cannot call function from non-function pointer type " +
        getNameString(lhs->getType()));

  std::vector<llvm::Value *> args = {rhs};
  return builder_.CreateCall(lhs, llvm::ArrayRef<llvm::Value *>(args));
}

llvm::Value *translator::apply_op(ast::binary_op<ast::at> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  auto *rval = rhs->getType()->isPointerTy() ? builder_.CreateLoad(rhs) : rhs;
  if (!rval->getType()->isIntegerTy()) {
    throw std::runtime_error("Array's index must be integer, not " +
                             getNameString(rval->getType()));
  }
  if (!lhs->getType()->isPointerTy()) {
    throw std::runtime_error("Cannot get element from non-pointer type " +
                             getNameString(lhs->getType()));
  }

  auto *lval = lhs;

  if (!lhs->getType()->getPointerElementType()->isArrayTy()) {
    lval = builder_.CreateLoad(lhs);

    if (!lval->getType()->getPointerElementType()->isArrayTy()) {
      throw std::runtime_error("Cannot get element from non-array type " +
                               getNameString(lhs->getType()));
    }
  }
  // Now lval's type is pointer to array

  std::vector<llvm::Value *> idxList = {builder_.getInt32(0), rval};
  auto *ep =
      builder_.CreateInBoundsGEP(lval->getType()->getPointerElementType(), lval,
                                 llvm::ArrayRef<llvm::Value *>(idxList));
  if (op.lval)
    return ep;
  else
    return builder_.CreateLoad(ep);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::load> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  if (!lhs->getType()->isPointerTy())
    throw std::runtime_error("Cannot load from non-pointer type " +
                             getNameString(lhs->getType()));
  return builder_.CreateLoad(lhs);
}

llvm::Value *translator::apply_op(ast::binary_op<ast::ret> const &op,
                                  llvm::Value *lhs, llvm::Value *rhs) {
  return builder_.CreateRet(lhs);
}

std::unique_ptr<module> module::create(parser::parsed const &tree, context &ctx,
                                       std::string const &name) {

  std::unique_ptr<llvm::Module> mod(new llvm::Module(name, ctx.llvmcontext));
  llvm::IRBuilder<> builder(mod->getContext());

  std::vector<llvm::Type *> args_type = {builder.getInt32Ty()};
  auto *main_func = llvm::Function::Create(
      llvm::FunctionType::get(builder.getInt32Ty(), args_type, false),
      llvm::Function::ExternalLinkage, "main", mod.get());

  builder.SetInsertPoint(
      llvm::BasicBlock::Create(mod->getContext(), "main_entry", main_func));

  translator tr(std::move(mod), builder);
  auto val = tr(tree);
  mod = tr.returnModule();

  std::vector<llvm::Value *> args = {builder.getInt32(0)};
  builder.CreateCall(val, llvm::ArrayRef<llvm::Value *>(args));

  builder.CreateRet(builder.getInt32(0));

  return std::unique_ptr<module>(new module(std::move(mod), val));
}

std::string module::irgen() {
  std::string result;
  llvm::raw_string_ostream stream(result);

  module_->print(stream, nullptr);
  return result;
}

llvm::GenericValue module::run() {
  llvm::InitializeNativeTarget();
  auto *funcptr = llvm::cast<llvm::Function>(val);
  std::unique_ptr<llvm::ExecutionEngine> engine(
      llvm::EngineBuilder(std::move(module_))
          .setEngineKind(llvm::EngineKind::Either)
          .create());
  engine->finalizeObject();
  auto res = engine->runFunction(funcptr, std::vector<llvm::GenericValue>(1));
  engine->removeModule(module_.get());
  return res;
}

}; // namespace assembly
}; // namespace scopion
