#ifndef SCOPION_PARSER_H_
#define SCOPION_PARSER_H_

#include "scopion/ast/ast.h"

namespace scopion {
namespace parser {

struct parsed {
  ast::expr ast;
  std::string const &code;
  parsed(ast::expr const &ast_, std::string const &code_)
      : ast(ast_), code(code_) {}
};

parsed parse(std::string const &code);

}; // namespace parser

}; // namespace scopion

#endif // SCOPION_PARSER_H_
