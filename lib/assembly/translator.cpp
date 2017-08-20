#include "scopion/assembly/translator.hpp"
#include "scopion/assembly/value.hpp"
#include "scopion/parser/parser.hpp"

#include "scopion/error.hpp"

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
  module_->getOrInsertFunction("GC_init", llvm::FunctionType::get(builder_.getVoidTy(), false));
  module_->getOrInsertFunction(
      "GC_malloc",
      llvm::FunctionType::get(builder_.getInt8Ty()->getPointerTo(),
                              llvm::ArrayRef<llvm::Type*>({builder_.getInt64Ty()}), false));
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

value* translator::operator()(ast::pre_variable const& astv)
{
  if (ast::attr(astv).lval)
    throw error("Pre-defined variables cannnot be assigned", ast::attr(astv).where, code_range_);

  auto const name = llvm::StringRef(ast::val(astv)).ltrim('@');
  if (auto fp = module_->getFunction(name))
    return new value(fp, astv);
  else {
    if (name.equals("import")) {
      auto itm = ast::attr(astv).attributes.find("m");
      auto iti = ast::attr(astv).attributes.find("ir");
      if (itm != ast::attr(astv).attributes.end()) {  // found path to module
        std::ifstream ifs(itm->second);
        if (ifs.fail()) {
          throw error("failed to open " + itm->second, ast::attr(astv).where, code_range_);
        }
        std::string code((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ifs.close();
        auto parsed = parser::parse(code);
        translator tr(module_, builder_, parsed.code);
        return boost::apply_visitor(tr, parsed.ast);
      } else if (iti != ast::attr(astv).attributes.end()) {  // found path to ir
        llvm::SMDiagnostic err;
        std::unique_ptr<llvm::Module> module =
            llvm::parseIRFile(iti->second, err, module_->getContext());
        if (!module) {
          llvm::raw_os_ostream stream(std::cerr);
          err.print(iti->second.c_str(), stream);
          throw error("Error happened during import of llvm ir", ast::attr(astv).where,
                      code_range_);
        }
        std::vector<llvm::Type*> fields;
        auto destv = new value(nullptr, astv);

        for (auto i = module->getFunctionList().begin(); i != module->getFunctionList().end();
             ++i) {
          llvm::Function* func;
          if (!(func = module_->getFunction(i->getName())))
            func = llvm::Function::Create(i->getFunctionType(), llvm::Function::ExternalLinkage,
                                          i->getName(), module_.get());
          auto vp                              = new value(func, astv);
          destv->symbols()[i->getName().str()] = vp;
          destv->fields()[i->getName().str()] = std::distance(module->getFunctionList().begin(), i);
          vp->setParent(destv);
          fields.push_back(i->getType());
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

        return destv;
      } else {
        throw error("Import path isn't specified", ast::attr(astv).where, code_range_);
      }
    } else if (name.equals("self")) {
      if (ast::attr(astv).lval)
        throw error("Assigning to @self is not allowed", ast::attr(astv).where, code_range_);

      auto v       = ast::variable("__self");
      ast::attr(v) = ast::attr(astv);
      return operator()(v);
    } else {
      throw error("Pre-defined variable \"" + name.str() + "\" is not defined",
                  ast::attr(astv).where, code_range_);
    }
  }
}

value* translator::operator()(ast::variable const& astv)
{
  try {
    auto vp = thisScope_->symbols().at(ast::val(astv));
    vp->setName(ast::val(astv));
    if (ast::attr(astv).lval || vp->isLazy() || !vp->isFundamental())
      return vp->copy();
    else
      return vp->copyWithNewLLVMValue(builder_.CreateLoad(vp->getLLVM()));
  } catch (std::out_of_range&) {
    if (ast::attr(astv).lval) {
      // not found in symbols & to be assigned -> declaration
      auto vp = new value(nullptr, astv);
      vp->setName(ast::val(astv));
      return vp;
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
  t              = firstelem->isFundamental() ? t : t->getPointerElementType();
  auto aryType   = llvm::ArrayType::get(t, ast::val(astv).size());
  auto aryPtr    = createGCMalloc(aryType);  // Allocate necessary memory
  auto destv     = new value(aryPtr, astv);

  std::vector<value*> values;
  for (auto const x : ast::val(astv) | boost::adaptors::indexed()) {
    auto v = x.index() == 0 ? firstelem : boost::apply_visitor(*this, x.value());
    if (v->getLLVM()->getType() != t) {
      throw error("all elements of array must have the same type", ast::attr(astv).where,
                  code_range_);
    }
    v->setParent(destv);
    destv->symbols()[std::to_string(x.index())] = v;
    // store value into fields list so that we can get this value later
    if (!v->isLazy())
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

  for (auto const& m : ast::val(astv) | boost::adaptors::indexed()) {
    auto vp                                     = boost::apply_visitor(*this, m.value().second);
    destv->symbols()[ast::val(m.value().first)] = vp;
    vp->setParent(destv);
    if (!vp->isLazy()) {
      fields.push_back(vp->isFundamental() ? vp->getLLVM()->getType()
                                           : vp->getLLVM()->getType()->getPointerElementType());
    }
  }

  llvm::StructType* structTy = llvm::StructType::create(module_->getContext());
  structTy->setBody(fields);

  auto ptr = createGCMalloc(structTy);
  destv->setLLVM(ptr);

  uint32_t i = 0;
  for (auto const& v : destv->symbols()) {
    if (!v.second->isLazy()) {
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
value* translator::operator()(ast::op<ast::odot, 2> const& op)
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
value* translator::operator()(ast::op<ast::ret, 1> const& op)
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
