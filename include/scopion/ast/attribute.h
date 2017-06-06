#ifndef SCOPION_AST_ATTRIBUTE_H_
#define SCOPION_AST_ATTRIBUTE_H_

#include <boost/range/iterator_range.hpp>

namespace scopion {
namespace ast {

struct attribute {
  boost::iterator_range<std::string::const_iterator> where;
  bool lval = false;
  bool to_call = false;
};
bool operator==(attribute const &lhs, attribute const &rhs);

}; // namespace ast
}; // namespace scopion

#endif
