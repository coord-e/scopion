/**
* @file translator.cpp
*
* (c) copyright 2017 coord.e
*
* This file is part of scopion.
*
* scopion is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* scopion is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with scopion.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "scopion/assembly/translator.hpp"
#include "scopion/assembly/value.hpp"
#include "scopion/parser/parser.hpp"

#include "scopion/config.hpp"
#include "scopion/error.hpp"

#include <llvm/AsmParser/Parser.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_os_ostream.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include <boost/range/adaptor/indexed.hpp>
#include <boost/range/iterator_range.hpp>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace scopion
{
namespace assembly
{
translator::translator()
    : boost::static_visitor<value*>(),
      module_(std::make_unique<module>("notafile")),
      builder_(module_->getContext()),
      thisScope_(new value())
{
}

translator::translator(boost::filesystem::path const& name,
                       std::vector<std::string> const& flags,
                       std::string const& tlfname)
    : boost::static_visitor<value*>(),
      module_(std::make_unique<module>(name.filename().string(), tlfname)),
      builder_(module_->getContext()),
      thisScope_(new value()),
      flags_(flags)
{
}

translator::translator(std::unique_ptr<module>&& module,
                       llvm::IRBuilder<>& builder,
                       std::vector<std::string> const& flags,
                       std::string const& tlfname)
    : boost::static_visitor<value*>(),
      module_(std::move(module)),
      builder_(builder),
      thisScope_(new value()),
      flags_(flags)
{
}

void translator::createMain()
{
  std::vector<llvm::Type*> args_type = {builder_.getInt32Ty(),
                                        builder_.getInt8PtrTy()->getPointerTo()};
  auto mainf                         = llvm::Function::Create(
      llvm::FunctionType::get(builder_.getInt32Ty(), args_type, false),
      llvm::Function::ExternalLinkage, module_->getTopFunctionName(), module_->getLLVMModule());
  auto mainbb = llvm::BasicBlock::Create(module_->getContext(), "entry", mainf);
  builder_.SetInsertPoint(mainbb);
}

value* translator::translateAST(ast::expr const& ast, error& err)
{
  try {
    return boost::apply_visitor(*this, ast);
  } catch (error& e) {
    err = e;
    return nullptr;
  }
}

llvm::Value* translator::createMainRet(value* val, error& err)
{
  auto* mainf = module_->getLLVMModule()->getFunction(module_->getTopFunctionName());
  assert(mainf && "main cannot be found in the module");

  if (!llvm::isa<llvm::Function>(val->getLLVM())) {
    auto where = ast::apply<locationInfo>(
        [](auto& x) -> locationInfo { return ast::attr(x).where; }, val->getAst());
    err = error("Top-level value must be function", where, errorType::Translate);
    return nullptr;
  }

  llvm::Value* llval;
  std::vector<llvm::Value*> arg_llvm_values;

  try {
    std::vector<value*> arg_values;
    if (!llvm::cast<llvm::Function>(val->getLLVM())->arg_empty()) {
      for (auto it = mainf->arg_begin(); it != mainf->arg_end(); it++) {
        arg_llvm_values.push_back(it);
        arg_values.push_back(new value(it, ast::expr{}));
      }
    }
    llval = evaluate(val, arg_values, *this)->getLLVM();
  } catch (error& e) {
    err = e;
    return nullptr;
  }

  llvm::Value* ret = builder_.CreateCall(llval, llvm::ArrayRef<llvm::Value*>(arg_llvm_values));

  builder_.CreateRet(ret->getType()->isVoidTy() ? builder_.getInt32(0) : ret);
  return llval;
}

void translator::insertGCInitInMain()
{
  auto ib = builder_.GetInsertBlock();
  auto ip = builder_.GetInsertPoint();
  builder_.SetInsertPoint(
      &(module_->getLLVMModule()->getFunction(module_->getTopFunctionName())->getEntryBlock()));
  builder_.CreateCall(module_->getLLVMModule()->getFunction("GC_init"),
                      llvm::ArrayRef<llvm::Value*>{});
  builder_.SetInsertPoint(ib, ip);
}

bool translator::hasFlag(std::string const& key)
{
  return std::find(flags_.cbegin(), flags_.cend(), key) != flags_.cend();
}

value* translator::import(std::string const& path, ast::pre_variable const& astv)
{
  auto thisp   = ast::attr(astv).where.getPath();
  auto abspath = boost::filesystem::absolute(
      path, thisp ? thisp->parent_path() : boost::filesystem::current_path());
  std::ifstream ifs(abspath.string());
  if (ifs.fail()) {
    return nullptr;
  }
  std::string code((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  ifs.close();
  error err;
  auto parsed = parser::parse(code, err, abspath);
  if (!parsed)
    throw err;
  translator tr(std::move(module_), builder_);
  auto val = boost::apply_visitor(tr, *parsed);
  module_  = tr.takeModule();
  return val;
}

value* translator::importIR(std::string const& path, ast::pre_variable const& astv)
{
  std::unique_ptr<llvm::Module> module;
  if (loaded_map_.count(path) != 0) {
    module = std::move(loaded_map_[path]);
  } else {
    llvm::SMDiagnostic err;
    module = llvm::parseIRFile(path, err, module_->getContext());
    if (!module) {
      llvm::raw_os_ostream stream(std::cerr);
      err.print(path.c_str(), stream);
      return nullptr;
    }
  }
  auto destv = new value(nullptr, astv);

  std::vector<llvm::Type*> fields;
  uint32_t cnt = 0;
  for (auto i = module->getFunctionList().begin(); i != module->getFunctionList().end(); ++i) {
    // FIXME: find better way to avoid link error (zopen causes link error)
    if (!i->getName().startswith("llvm.")) {
      llvm::Function* func;
      if (!(func = module_->getLLVMModule()->getFunction(i->getName())))
        func = llvm::Function::Create(i->getFunctionType(), llvm::Function::ExternalLinkage,
                                      i->getName(), module_->getLLVMModule());
      auto vp                              = new value(func, astv);
      destv->symbols()[i->getName().str()] = vp;
      destv->fields()[i->getName().str()]  = cnt;
      cnt++;
      vp->setParent(destv);
      fields.push_back(i->getType());
    }
  }

  llvm::StructType* structTy = llvm::StructType::create(module_->getContext());
  structTy->setBody(fields);

  auto ptr = builder_.CreateAlloca(structTy);
  destv->setLLVM(ptr);

  for (auto const& x : destv->fields()) {
    auto p = builder_.CreateStructGEP(structTy, ptr, x.second);
    builder_.CreateStore(destv->symbols()[x.first]->getLLVM(), p);
    destv->symbols()[x.first]->setLLVM(p);
  }

  loaded_map_[path] = std::move(module);

  return destv;
}

value* translator::importCHeader(std::string const& path, ast::pre_variable const& astv)
{
  auto h2irpath = std::string(std::getenv("HOME")) + "/" SCOPION_CACHE_DIR "/h2ir/";
  system((std::string("bash " SCOPION_ETC_DIR "/h2ir/scopion-h2ir ") + path).c_str());
  return importIR(h2irpath + path, astv);
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
    throw error("An integer constant is not to be assigned", ast::attr(astv).where,
                errorType::Translate);

  if (ast::attr(astv).to_call)
    throw error("An integer constant is not to be called", ast::attr(astv).where,
                errorType::Translate);

  return new value(llvm::ConstantInt::getSigned(builder_.getInt32Ty(), ast::val(astv)), astv);
}

value* translator::operator()(ast::decimal astv)
{
  if (ast::attr(astv).lval)
    throw error("An integer constant is not to be assigned", ast::attr(astv).where,
                errorType::Translate);

  if (ast::attr(astv).to_call)
    throw error("An integer constant is not to be called", ast::attr(astv).where,
                errorType::Translate);

  return new value(llvm::ConstantFP::get(builder_.getDoubleTy(), ast::val(astv)), astv);
}

value* translator::operator()(ast::boolean astv)
{
  if (ast::attr(astv).lval)
    throw error("A boolean constant is not to be assigned", ast::attr(astv).where,
                errorType::Translate);

  if (ast::attr(astv).to_call)
    throw error("A boolean constant is not to be called", ast::attr(astv).where,
                errorType::Translate);

  return new value(llvm::ConstantInt::get(builder_.getInt1Ty(), ast::val(astv)), astv);
}

value* translator::operator()(ast::string const& astv)
{
  if (ast::attr(astv).lval)
    throw error("A string constant is not to be assigned", ast::attr(astv).where,
                errorType::Translate);

  if (ast::attr(astv).to_call)
    throw error("A string constant is not to be called", ast::attr(astv).where,
                errorType::Translate);

  return new value(builder_.CreateGlobalStringPtr(ast::val(astv)), astv);
}

value* translator::operator()(ast::pre_variable const& astv)
{
  if (ast::attr(astv).lval)
    throw error("Pre-defined variables cannnot be assigned", ast::attr(astv).where,
                errorType::Translate);

  auto const name = llvm::StringRef(ast::val(astv)).ltrim('@');
  if (auto fp = module_->getLLVMModule()->getFunction(name))
    return new value(fp, astv);
  else {
    if (name.equals("import")) {
      auto itm = ast::attr(astv).attributes.find("m");
      auto iti = ast::attr(astv).attributes.find("ir");
      auto its = ast::attr(astv).attributes.find("c");
      auto itl = ast::attr(astv).attributes.find("link");
      if (itm != ast::attr(astv).attributes.end()) {  // found path to module
        if (auto v = import(itm->second, astv))
          return v;
        else
          throw error("Failed to open " + itm->second, ast::attr(astv).where, errorType::Translate);
      } else if (iti != ast::attr(astv).attributes.end()) {  // found path to ir
        if (itl != ast::attr(astv).attributes.end())
          module_->link_libraries_.push_back(itl->second);
        if (auto v = importIR(iti->second, astv))
          return v;
        else
          throw error("Error happened during import of llvm ir", ast::attr(astv).where,
                      errorType::Translate);
      } else if (its != ast::attr(astv).attributes.end()) {  // found path to c header
        if (itl != ast::attr(astv).attributes.end())
          module_->link_libraries_.push_back(itl->second);
        if (auto v = importCHeader(its->second, astv))
          return v;
        else
          throw error("Error happened during import of a c header", ast::attr(astv).where,
                      errorType::Translate);
      } else {
        throw error("Import path isn't specified", ast::attr(astv).where, errorType::Translate);
      }
    } else if (name.equals("self")) {
      if (ast::attr(astv).lval)
        throw error("Assigning to @self is not allowed", ast::attr(astv).where,
                    errorType::Translate);

      auto v       = ast::variable("__self");
      ast::attr(v) = ast::attr(astv);
      return operator()(v);
    } else {
      throw error("Pre-defined variable \"" + name.str() + "\" is not defined",
                  ast::attr(astv).where, errorType::Translate);
    }
  }
}

value* translator::operator()(ast::variable const& astv)
{
  auto it = thisScope_->symbols().find(ast::val(astv));
  if (it == thisScope_->symbols().end()) {
    if (ast::attr(astv).lval) {
      // not found in symbols & to be assigned -> declaration
      auto vp = new value(nullptr, astv);
      vp->setName(ast::val(astv));
      return vp;
    } else {
      throw error("\"" + ast::val(astv) + "\" has not declared in this scope",
                  ast::attr(astv).where, errorType::Translate);
    }
  } else {
    auto vp = it->second;
    vp->setName(ast::val(astv));
    if (ast::attr(astv).lval || vp->getType()->isLazy() || !vp->getType()->isFundamental())
      return vp->copy();
    else
      return vp->copyWithNewLLVMValue(builder_.CreateLoad(vp->getLLVM()));
  }
}

value* translator::operator()(ast::identifier const& astv)
{
  return new value();  // void
}

value* translator::operator()(ast::struct_key const& astv)
{
  return new value();  // void
}

value* translator::operator()(ast::array const& astv)
{
  if (ast::attr(astv).lval)
    throw error("An array constant is not to be assigned", ast::attr(astv).where,
                errorType::Translate);

  if (ast::attr(astv).to_call)
    throw error("An array constant is not to be called", ast::attr(astv).where,
                errorType::Translate);

  auto firstelem = boost::apply_visitor(*this, ast::val(astv)[0]);
  auto t         = ast::val(astv).empty() ? builder_.getVoidTy() : firstelem->getType()->getLLVM();
  t              = firstelem->getType()->isFundamental() ? t : t->getPointerElementType();
  auto aryType   = llvm::ArrayType::get(t, ast::val(astv).size());
  auto aryPtr    = builder_.CreateAlloca(aryType);  // Allocate necessary memory
  auto destv     = new value(aryPtr, astv);

  std::vector<value*> values;
  for (auto const x : ast::val(astv) | boost::adaptors::indexed()) {
    auto v = x.index() == 0 ? firstelem : boost::apply_visitor(*this, x.value());
    if (v->getType()->getLLVM() != t) {
      throw error("all elements of array must have the same type", ast::attr(astv).where,
                  errorType::Translate);
    }
    v->setParent(destv);
    destv->symbols()[std::to_string(x.index())] = v;
    // store value into fields list so that we can get this value later
    if (!v->getType()->isLazy())
      values.push_back(v);
  }

  for (auto const v : values | boost::adaptors::indexed()) {
    std::vector<llvm::Value*> idxList = {builder_.getInt32(0),
                                         builder_.getInt32(static_cast<uint32_t>(v.index()))};
    auto p = builder_.CreateInBoundsGEP(aryType, aryPtr, llvm::ArrayRef<llvm::Value*>(idxList));
    std::string str = std::to_string(v.index());
    // not good way...? (many to_string)

    if (!copyFull(v.value(), new value(p, v.value()->getAst()), str, p, destv)) {
      assert(false && "Assigned with wrong type during construction of the structure");
    }
  }
  return destv;
}

value* translator::operator()(ast::arglist const& astv)
{
  return new value();  // void
}

value* translator::operator()(ast::structure const& astv)
{
  std::vector<llvm::Type*> fields;
  auto destv = new value(nullptr, astv);

  for (auto const m : ast::val(astv) | boost::adaptors::indexed()) {
    auto vp                                     = boost::apply_visitor(*this, m.value().second);
    destv->symbols()[ast::val(m.value().first)] = vp;
    vp->setParent(destv);
    if (!vp->getType()->isLazy()) {
      fields.push_back(vp->getType()->isFundamental()
                           ? vp->getType()->getLLVM()
                           : vp->getType()->getLLVM()->getPointerElementType());
    }
  }

  llvm::StructType* structTy = llvm::StructType::create(module_->getContext());
  structTy->setBody(fields);
  structTy->setName("user_type");

  for (auto t : module_->getLLVMModule()->getIdentifiedStructTypes()) {
    if (structTy->isLayoutIdentical(t)) {
      structTy = t;
      break;
    }
  }

  auto ptr = builder_.CreateAlloca(structTy);
  destv->setLLVM(ptr);

  uint32_t i = 0;
  for (auto const& v : destv->symbols()) {
    if (!v.second->getType()->isLazy()) {
      destv->fields()[v.first] = i;
      auto p                   = builder_.CreateStructGEP(structTy, ptr, i);
      if (!copyFull(v.second, new value(p, v.second->getAst()), v.first, p, destv)) {
        assert(false && "Assigned with wrong type during construction of the structure");
      }
      i++;
    }
  }

  return destv;
}

value* translator::operator()(ast::function const& fcv)
{
  if (ast::attr(fcv).lval)
    throw error("A function constant is not to be assigned", ast::attr(fcv).where,
                errorType::Translate);

  std::string func_name =
      ast::attr(fcv).attributes.find("export") != ast::attr(fcv).attributes.end()
          ? ast::attr(fcv).attributes.at("export")
          : "";

  auto& args = ast::val(fcv).first;
  if (std::all_of(args.begin(), args.end(),
                  [](auto& x) {
                    return ast::attr(x).attributes.find("type") != ast::attr(x).attributes.end();
                  }) &&
      ast::attr(fcv).attributes.find("rettype") != ast::attr(fcv).attributes.end() &&
      ast::attr(fcv).attributes.find("lazy") == ast::attr(fcv).attributes.end()) {
    std::vector<llvm::Type*> arg_types;

    auto parse = [this](auto& x, std::string const& key = "type") {
      llvm::SMDiagnostic err;
      auto type_name = ast::attr(x).attributes.at(key);
      auto t         = llvm::parseType(type_name, err, *(module_->getLLVMModule()));
      if (!t) {
        llvm::raw_os_ostream stream(std::cerr);
        err.print("", stream);
        throw error("Failed to parse type name \"" + type_name + "\"", ast::attr(x).where,
                    errorType::Translate);
      }
      return t;
    };
    std::transform(args.begin(), args.end(), std::back_inserter(arg_types), parse);

    auto ret_type = parse(fcv, "rettype");

    llvm::FunctionType* func_type =
        llvm::FunctionType::get(ret_type, std::vector<llvm::Type*>(arg_types), false);
    llvm::Function* func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage,
                                                  func_name, module_->getLLVMModule());

    llvm::BasicBlock* newentry = llvm::BasicBlock::Create(module_->getContext(), "entry", func);

    auto prevScope = thisScope_;

    auto pb = builder_.GetInsertBlock();
    auto pp = builder_.GetInsertPoint();

    thisScope_ = new value(newentry, fcv);
    builder_.SetInsertPoint(newentry);

    auto selfptr                    = builder_.CreateAlloca(func->getType(), nullptr, "__self");
    thisScope_->symbols()["__self"] = new value(selfptr, fcv);
    builder_.CreateStore(func, selfptr);

    auto it = func->arg_begin();
    for (auto const arg_name : ast::val(fcv).first | boost::adaptors::indexed()) {
      auto name = ast::val(arg_name.value());
      auto aptr =
          builder_.CreateAlloca(arg_types[static_cast<unsigned long>(arg_name.index())], nullptr,
                                name);  // declare arguments
      thisScope_->symbols()[name] = new value(aptr, arg_name.value());
      auto tmpval                 = new value(&(*it), arg_name.value());
      builder_.CreateStore(tmpval->getType()->isFundamental() ? static_cast<llvm::Value*>(&(*it))
                                                              : builder_.CreateLoad(&(*it)),
                           aptr);
      delete tmpval;
      it++;
    }

    for (auto const& line : ast::val(fcv).second) {
      boost::apply_visitor(*this, line);
    }

    if (ret_type->isVoidTy()) {
      builder_.CreateRetVoid();
    }

    thisScope_ = prevScope;
    builder_.SetInsertPoint(pb, pp);

    return new value(func, fcv);
  } else {  // lazy evaluation route
    llvm::FunctionType* func_type = llvm::FunctionType::get(
        builder_.getVoidTy(), std::vector<llvm::Type*>(args.size(), builder_.getInt32Ty()), false);
    llvm::Function* func =
        llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, func_name, nullptr);

    auto destv = new value(func, fcv, true);  // is_lazy = true

    return destv;
  }
}

value* translator::operator()(ast::scope const& scv)
{
  if (ast::attr(scv).lval)
    throw error("A scope constant is not to be assigned", ast::attr(scv).where,
                errorType::Translate);

  auto bb = llvm::BasicBlock::Create(module_->getContext());  // empty

  auto destv = new value(bb, scv, true);  // is_lazy = true
  destv->symbols().insert(thisScope_->symbols().begin(), thisScope_->symbols().end());

  return destv;
}

}  // namespace assembly
}  // namespace scopion
