#ifndef SCOPION_PARSER_H_
#define SCOPION_PARSER_H_

#include "AST/AST.h"

namespace scopion{
namespace parser {

    std::vector< ast::expr > parse(std::string const& code);

}; // namespace parser
}; //namespace scopion

#endif //SCOPION_PARSER_H_
