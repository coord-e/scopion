#ifndef SCOPION_AST_VALUE_WRAPPER_H_
#define SCOPION_AST_VALUE_WRAPPER_H_

#include "scopion/ast/attribute.hpp"

namespace scopion {
namespace ast {

template <typename T> class value_wrapper {
public:
  T value;
  attribute attr;

  value_wrapper(T const &val) : value(val) {}
  value_wrapper() : value(T()) {}

  operator T() const { return value; }
};
template <class T>
bool operator==(value_wrapper<T> const &lhs, value_wrapper<T> const &rhs);

}; // namespace ast
}; // namespace scopion

#endif
