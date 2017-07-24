#include "scopion/assembly/translator.hpp"
#include "scopion/assembly/value.hpp"

#include "scopion/error.hpp"

#include <algorithm>
#include <map>
#include <string>

#include <boost/range/adaptor/indexed.hpp>
#include <boost/range/iterator_range.hpp>

namespace scopion {
namespace assembly {

translator::translator(std::shared_ptr<llvm::Module> &module,
                       llvm::IRBuilder<> &builder, std::string const &code)
    : boost::static_visitor<value_t>(), module_(module), builder_(builder),
      code_range_(boost::make_iterator_range(code.begin(), code.end())) {
  auto ib = builder_.GetInsertBlock();
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
  builder_.SetInsertPoint(ib);
}

value_t translator::operator()(ast::value value) {
  return boost::apply_visitor(*this, value);
}

value_t translator::operator()(ast::operators value) {
  return boost::apply_visitor(*this, value);
}

value_t translator::operator()(ast::integer value) {
  if (ast::attr(value).lval)
    throw error("An integer constant is not to be assigned",
                ast::attr(value).where, code_range_);

  if (ast::attr(value).to_call)
    throw error("An integer constant is not to be called",
                ast::attr(value).where, code_range_);

  return llvm::ConstantInt::get(builder_.getInt32Ty(), ast::val(value));
}

value_t translator::operator()(ast::boolean value) {
  if (ast::attr(value).lval)
    throw error("A boolean constant is not to be assigned",
                ast::attr(value).where, code_range_);

  if (ast::attr(value).to_call)
    throw error("A boolean constant is not to be called",
                ast::attr(value).where, code_range_);

  return llvm::ConstantInt::get(builder_.getInt1Ty(), ast::val(value));
}

value_t translator::operator()(ast::string const &value) {
  if (ast::attr(value).lval)
    throw error("A string constant is not to be assigned",
                ast::attr(value).where, code_range_);

  if (ast::attr(value).to_call)
    throw error("A string constant is not to be called", ast::attr(value).where,
                code_range_);

  return builder_.CreateGlobalStringPtr(ast::val(value));
}

value_t translator::operator()(ast::variable const &value) {
  if (ast::attr(value).to_call) { // in case of function
    // find globally declared function
    auto valp = module_->getFunction(ast::val(value));
    if (valp != nullptr) { // found
      return valp;
    } else {
      try {
        auto varp = getSymbols(currentScope_).at(ast::val(value));

        if (varp.type() == typeid(llvm::Value *)) { // llvm value
          auto vval = get_v(varp, value);
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
              return builder_.CreateLoad(vval);
            }
          }
          throw error(
              "Variable \"" + ast::val(value) + "\" is not a function but " +
                  getNameString(vval->getType()->getPointerElementType()),
              ast::attr(value).where, code_range_);
        } else // lazy value
          return varp;
      } catch (std::out_of_range &) {
        throw error("Function \"" + ast::val(value) +
                        "\" has not declared in this scope",
                    ast::attr(value).where, code_range_);
      }
    }
  } else { // not function (normal variable)
    try {
      auto valp = getSymbols(currentScope_).at(ast::val(value));
      if (ast::attr(value).lval) { // to be assigned
        return valp;
      } else {
        if (valp.type() == typeid(llvm::Value *))
          return builder_.CreateLoad(get_v(valp, value));
        else // lazy value
          return valp;
      }
    } catch (std::out_of_range &) {
      if (ast::attr(value).lval) {
        // not found in symbols & to be assigned -> declaration
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

value_t translator::operator()(ast::identifier const &value) { return nullptr; }

value_t translator::operator()(ast::array const &value) {
  if (ast::attr(value).lval)
    throw error("An array constant is not to be assigned",
                ast::attr(value).where, code_range_);

  if (ast::attr(value).to_call)
    throw error("An array constant is not to be called", ast::attr(value).where,
                code_range_);

  auto &ary = ast::val(value);
  std::vector<llvm::Value *> values;
  for (auto const &elm : ary) {
    // Convert exprs to llvm::Value* and store it into new vector
    value_t v = boost::apply_visitor(*this, elm);
    values.push_back(get_v(v, value));
  }

  auto t = values.empty() ? builder_.getVoidTy() : values[0]->getType();
  // check if the type of all elements of array is same
  if (!std::all_of(values.begin(), values.end(),
                   [&t](auto &&v) { return t == v->getType(); }))
    throw error("all elements of array must have the same type",
                ast::attr(value).where, code_range_);

  auto aryType = llvm::ArrayType::get(t, ary.size());
  auto aryPtr = builder_.CreateAlloca(aryType); // Allocate necessary memory
  for (auto const v : values | boost::adaptors::indexed()) {
    std::vector<llvm::Value *> idxList = {
        builder_.getInt32(0),
        builder_.getInt32(static_cast<uint32_t>(v.index()))};
    auto p = builder_.CreateInBoundsGEP(aryType, aryPtr,
                                        llvm::ArrayRef<llvm::Value *>(idxList));

    builder_.CreateStore(get_v(v.value(), value), p);
  }
  return aryPtr;
}

value_t translator::operator()(ast::arglist const &value) { return nullptr; }

value_t translator::operator()(ast::structure const &value) {
  std::vector<value_t> vals;
  std::vector<llvm::Type *> fields;
  auto structName = createNewStructName();

  for (auto const &m : ast::val(value)) {
    fields_map[structName].push_back(ast::val(m.first));
    auto vp = get_v(boost::apply_visitor(*this, m.second), value);
    fields.push_back(vp->getType());

    vals.push_back(vp);
  }

  llvm::StructType *structTy =
      llvm::StructType::create(module_->getContext(), structName);
  structTy->setBody(fields);

  auto ptr = builder_.CreateAlloca(structTy);
  for (auto const v : vals | boost::adaptors::indexed()) {
    auto p = builder_.CreateStructGEP(structTy, ptr,
                                      static_cast<uint32_t>(v.index()));
    builder_.CreateStore(get_v(v.value(), value), p);
  }

  return ptr;
}

value_t translator::operator()(ast::function const &fcv) {
  if (ast::attr(fcv).lval)
    throw error("A function constant is not to be assigned",
                ast::attr(fcv).where, code_range_);

  auto &args = ast::val(fcv).first;
  auto &lines = ast::val(fcv).second;

  llvm::FunctionType *func_type = llvm::FunctionType::get(
      builder_.getVoidTy(),
      std::vector<llvm::Type *>(args.size(), builder_.getInt32Ty()), false);
  llvm::Function *func =
      llvm::Function::Create(func_type, llvm::Function::ExternalLinkage);
  auto lv = lazy_value<llvm::Function>(func, lines);
  for (auto const arg : args | boost::adaptors::indexed()) {
    lv.symbols[std::to_string(arg.index()) + ":" + ast::val(arg.value())] =
        nullptr; // add number prefixes to remember the order
  }
  return lv;
}

value_t translator::operator()(ast::scope const &scv) {
  auto bb = llvm::BasicBlock::Create(module_->getContext()); // empty
  auto newsc = lazy_value<llvm::BasicBlock>(bb, ast::val(scv));

  std::copy(getSymbols(currentScope_).begin(), getSymbols(currentScope_).end(),
            std::inserter(newsc.symbols, newsc.symbols.end()));

  return newsc;
}

bool translator::apply_bb(value_t v) {
  assert(v.type() == typeid(lazy_value<llvm::BasicBlock>));
  currentScope_ = v;
  auto vv = boost::get<lazy_value<llvm::BasicBlock>>(v);
  auto insts = vv.getInsts();
  for (auto it = insts.begin(); it != insts.end(); it++) {
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
