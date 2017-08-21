#ifndef SCOPION_ASSEMBLY_MODULE_H_
#define SCOPION_ASSEMBLY_MODULE_H_

#include "scopion/assembly/translator.hpp"
#include "scopion/assembly/value.hpp"
#include "scopion/context.hpp"

#include "scopion/parser/parser.hpp"

#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/Module.h>

#include <boost/optional.hpp>

namespace scopion
{
namespace assembly
{
class module
{
  friend boost::optional<module> translate(ast::expr const& astv,
                                           std::string const& filename,
                                           context& ctx,
                                           error& err);

  std::shared_ptr<llvm::Module> module_;
  value* val_;
  module(std::shared_ptr<llvm::Module>& module, value* val);

public:
  std::shared_ptr<llvm::Module> getLLVMModule();
  value* getValue();
  std::string getIR();
  void optimize(uint8_t optLevel = 3, uint8_t sizeLevel = 0);
};

boost::optional<module> translate(ast::expr const& astv,
                                  std::string const& filename,
                                  context& ctx,
                                  error& err);

};  // namespace assembly
};  // namespace scopion

#endif
