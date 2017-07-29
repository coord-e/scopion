#ifndef SCOPION_AST_VALUE_WRAPPER_H_
#define SCOPION_AST_VALUE_WRAPPER_H_

#include "scopion/ast/attribute.hpp"

#include <type_traits>

namespace scopion
{
namespace ast
{
template <typename T>
class value_wrapper
{
public:
  T value;
  attribute attr;

  value_wrapper(T const& val) : value(val) {}
  template <typename U,
            std::enable_if_t<std::is_constructible<T, U>::value>* = nullptr>
  value_wrapper(U const& val) : value(T(val))
  {
  }
  value_wrapper() : value(T()) {}
};
template <class T>
bool operator==(value_wrapper<T> const& lhs, value_wrapper<T> const& rhs);
template <class T>
bool operator<(value_wrapper<T> const& lhs, value_wrapper<T> const& rhs)
{
  return lhs.value < rhs.value;
}

};  // namespace ast
};  // namespace scopion

#endif
