#include "scopion/assembly/scoped_value.hpp"
#include "scopion/assembly/translator.hpp"

#include "scopion/error.hpp"

#include <algorithm>

#include <boost/range/adaptor/indexed.hpp>
#include <boost/range/iterator_range.hpp>

namespace scopion {
namespace assembly {

translator::translator(std::shared_ptr<llvm::Module> &module,
                       llvm::IRBuilder<> const &builder,
                       std::string const &code)
    : boost::static_visitor<scoped_value *>(), module_(module),
      builder_(builder),
      code_range_(boost::make_iterator_range(code.begin(), code.end())) {
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

scoped_value *translator::operator()(ast::value value) {
  return boost::apply_visitor(*this, value);
}

scoped_value *translator::operator()(ast::operators value) {
  return boost::apply_visitor(*this, value);
}

scoped_value *translator::operator()(ast::integer value) {
  if (ast::attr(value).lval)
    throw error("An integer constant is not to be assigned",
                ast::attr(value).where, code_range_);

  if (ast::attr(value).to_call)
    throw error("An integer constant is not to be called",
                ast::attr(value).where, code_range_);

  return new scoped_value(
      llvm::ConstantInt::get(builder_.getInt32Ty(), ast::val(value)));
}

scoped_value *translator::operator()(ast::boolean value) {
  if (ast::attr(value).lval)
    throw error("A boolean constant is not to be assigned",
                ast::attr(value).where, code_range_);

  if (ast::attr(value).to_call)
    throw error("A boolean constant is not to be called",
                ast::attr(value).where, code_range_);

  return new scoped_value(
      llvm::ConstantInt::get(builder_.getInt1Ty(), ast::val(value)));
}

scoped_value *translator::operator()(ast::string const &value) {
  if (ast::attr(value).lval)
    throw error("A string constant is not to be assigned",
                ast::attr(value).where, code_range_);

  if (ast::attr(value).to_call)
    throw error("A string constant is not to be called", ast::attr(value).where,
                code_range_);

  return new scoped_value(builder_.CreateGlobalStringPtr(ast::val(value)));
}

scoped_value *translator::operator()(ast::variable const &value) {
  if (ast::attr(value).to_call) {
    auto valp = module_->getFunction(ast::val(value));
    if (valp != nullptr) {
      return new scoped_value(valp);
    } else {
      try {
        auto varp = currentScope_->symbols.at(ast::val(value));

        if (varp->hasBlock()) {
          return varp;
        } else {
          auto vval = varp->getValue();
          if (!vval->getType()->isPointerTy())
            throw error("Variable \"" + ast::val(value) +
                            "\" should be a pointer but " +
                            getNameString(vval->getType()),
                        ast::attr(value).where, code_range_);
          if (vval->getType()->getPointerElementType()->isPointerTy()) {
            if (vval->getType()
                    ->getPointerElementType()
                    ->getPointerElementType()
                    ->isFunctionTy()) {
              return new scoped_value(builder_.CreateLoad(vval));
            }
          }
          throw error(
              "Variable \"" + ast::val(value) + "\" is not a function but " +
                  getNameString(vval->getType()->getPointerElementType()),
              ast::attr(value).where, code_range_);
        }
      } catch (std::out_of_range &) {
        throw error("Function \"" + ast::val(value) +
                        "\" has not declared in this scope",
                    ast::attr(value).where, code_range_);
      }
    }
  } else {
    try {
      auto valp = currentScope_->symbols.at(ast::val(value));
      if (ast::attr(value).lval) {
        return valp;
      } else {
        return new scoped_value(builder_.CreateLoad(valp->getValue()));
      }
    } catch (std::out_of_range &) {
      if (ast::attr(value).lval) {
        return nullptr;
      } else {
        throw error("\"" + ast::val(value) +
                        "\" has not declared in this scope",
                    ast::attr(value).where, code_range_);
      }
    }
  }
  assert(false);
}
scoped_value *translator::operator()(ast::array const &value) {
  if (ast::attr(value).lval)
    throw error("An array constant is not to be assigned",
                ast::attr(value).where, code_range_);

  if (ast::attr(value).to_call)
    throw error("An array constant is not to be called", ast::attr(value).where,
                code_range_);

  auto &ary = ast::val(value);
  std::vector<scoped_value *> values;
  for (auto const &elm : ary) {
    // Convert exprs to llvm::Value* and store it into new vector
    values.push_back(boost::apply_visitor(*this, elm));
  }

  auto t = values.empty() ? builder_.getVoidTy() : values[0]->getType();
  // check if the type of all elements of array is same
  if (!std::all_of(values.begin(), values.end(),
                   [&t](auto &&v) { return t == v->getType(); }))
    throw error("all elements of array must have the same type",
                ast::attr(value).where, code_range_);

  auto aryType = llvm::ArrayType::get(t, ary.size());
  auto aryPtr = builder_.CreateAlloca(aryType); // Allocate necessary memory
  for (auto const &v : values | boost::adaptors::indexed()) {
    std::vector<llvm::Value *> idxList = {builder_.getInt32(0),
                                          builder_.getInt32(v.index())};
    auto p = builder_.CreateInBoundsGEP(aryType, aryPtr,
                                        llvm::ArrayRef<llvm::Value *>(idxList));

    builder_.CreateStore(v.value()->getValue(), p);
  }
  return new scoped_value(aryPtr);
}

scoped_value *translator::operator()(ast::arglist const &value) {
  return nullptr;
}

scoped_value *translator::operator()(ast::function const &fcv) {
  if (ast::attr(fcv).lval)
    throw error("A function constant is not to be assigned",
                ast::attr(fcv).where, code_range_);

  auto &args = ast::val(fcv).first;
  auto &lines = ast::val(fcv).second;

  std::vector<llvm::Type *> func_args_type(args.size(), builder_.getInt32Ty());
  llvm::FunctionType *func_type =
      llvm::FunctionType::get(builder_.getVoidTy(), func_args_type, false);
  llvm::Function *func = llvm::Function::Create(
      func_type, llvm::Function::ExternalLinkage, "", module_.get());
  llvm::BasicBlock *entry =
      llvm::BasicBlock::Create(module_->getContext(),
                               "entryf", // BasicBlockの名前
                               func);
  auto prevScope = std::move(currentScope_);
  currentScope_ = new scoped_value(entry, &lines);

  auto pb = builder_.GetInsertBlock();
  auto pp = builder_.GetInsertPoint();

  builder_.SetInsertPoint(entry); // entryの開始

  currentScope_->symbols["self"] =
      new scoped_value{builder_.CreateAlloca(func->getType(), nullptr, "self")};

  for (auto const &v : args) {
    currentScope_->symbols[ast::val(v)] =
        new scoped_value{builder_.CreateAlloca(
            builder_.getInt32Ty(), nullptr, ast::val(v))}; // declare arguments
  }

  for (auto const &line : lines) {
    boost::apply_visitor(*this,
                         line); // If lines has function definition, the
                                // function will be defined twice.
  }

  llvm::Type *ret_type = nullptr;
  for (auto const &bb : *func) {
    for (auto itr = bb.getInstList().begin(); itr != bb.getInstList().end();
         ++itr) {
      if ((*itr).getOpcode() == llvm::Instruction::Ret) {
        if ((*itr).getOperand(0)->getType() != ret_type) {
          if (ret_type == nullptr) {
            ret_type = (*itr).getOperand(0)->getType();
          } else {
            throw error("All return values must have the same type",
                        ast::attr(fcv).where, code_range_);
          }
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
  currentScope_ = new scoped_value(newentry, &lines);
  builder_.SetInsertPoint(newentry);

  auto selfptr = builder_.CreateAlloca(newfunc->getType(), nullptr, "self");
  currentScope_->symbols["self"] = new scoped_value{selfptr};
  builder_.CreateStore(newfunc, selfptr);

  auto it = newfunc->getArgumentList().begin();
  for (auto const &v : args) {
    auto aptr = builder_.CreateAlloca(builder_.getInt32Ty(), nullptr,
                                      ast::val(v)); // declare arguments
    currentScope_->symbols[ast::val(v)] = new scoped_value{aptr};
    builder_.CreateStore(&(*it), aptr);
    it++;
  }

  for (auto const &line : lines) {
    boost::apply_visitor(*this, line);
  }

  if (ret_type == builder_.getVoidTy()) {
    builder_.CreateRetVoid();
  }

  currentScope_ = std::move(prevScope);
  builder_.SetInsertPoint(pb, pp); // entryを抜ける

  return new scoped_value(newfunc);
}

scoped_value *translator::operator()(ast::scope const &scv) {
  auto bb = llvm::BasicBlock::Create(module_->getContext()); // empty
  auto newsc = new scoped_value(bb, new std::vector<ast::expr>(ast::val(scv)));

  std::copy(currentScope_->symbols.begin(), currentScope_->symbols.end(),
            std::inserter(newsc->symbols, newsc->symbols.end()));

  return newsc;
}

bool translator::apply_bb(scoped_value *v) {
  assert(v->hasBlock());
  currentScope_ = v;
  auto insts = v->getInsts();
  decltype(insts->begin()) it;
  for (it = insts->begin(); it != insts->end(); it++) {
    boost::apply_visitor(*this, *it);
  }
  auto cb = builder_.GetInsertBlock();
  return std::none_of(cb->begin(), cb->end(), [](auto &i) {
    return i.getOpcode() == llvm::Instruction::Ret ||
           i.getOpcode() == llvm::Instruction::Br;
  });
}

}; // namespace assembly
}; // namespace scopion
