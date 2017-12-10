/**
* @file expr.hpp
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

#ifndef SCOPION_AST_EXPR_H_
#define SCOPION_AST_EXPR_H_

#include "scopion/ast/operators.hpp"
#include "scopion/ast/value.hpp"

#include <algorithm>
#include <array>
#include <initializer_list>
#include <iostream>
#include <type_traits>

namespace scopion
{
namespace ast
{
using expr_base = boost::variant<value, operators>;

struct expr : expr_base {
  using expr_base::expr_base;
  using expr_base::operator=;

  expr(expr_base const& ex) : expr_base(ex) {}
};
bool operator==(expr const& lhs, expr const& rhs);

template <class Op, size_t N>
class op_base
{
  using exprs_t = std::array<expr, N>;
  exprs_t exprs_;

public:
  op_base(std::initializer_list<expr> args)
  {
    assert(N == args.size() && "Initialization with wrong number of expression");
    std::copy(args.begin(), args.end(), exprs_.begin());
  }
  op_base(exprs_t const& args) : exprs_(args) {}

  expr const& operator[](size_t idx) const { return exprs_[idx]; }
  expr& operator[](size_t idx) { return exprs_[idx]; }
  expr const& at(size_t idx) const { return exprs_.at(idx); }
  expr& at(size_t idx) { return exprs_.at(idx); }

  expr* data() noexcept { return exprs_.data(); }
  const expr* data() const noexcept { return exprs_.data(); }

  constexpr bool empty() const noexcept { return !N; }

  constexpr size_t size() const noexcept { return N; }

  using iterator               = typename exprs_t::iterator;
  using const_iterator         = typename exprs_t::const_iterator;
  using reverse_iterator       = typename exprs_t::reverse_iterator;
  using const_reverse_iterator = typename exprs_t::const_reverse_iterator;

  iterator begin() noexcept { return exprs_.begin(); }
  const_iterator begin() const noexcept { return exprs_.cbegin(); }

  iterator end() noexcept { return exprs_.end(); }
  const_iterator end() const noexcept { return exprs_.cend(); }

  const_iterator cbegin() const noexcept { return exprs_.cbegin(); }

  const_iterator cend() const noexcept { return exprs_.cend(); }

  reverse_iterator rbegin() noexcept { return exprs_.rbegin(); }
  const_reverse_iterator rbegin() const noexcept { return exprs_.crbegin(); }

  reverse_iterator rend() noexcept { return exprs_.rend(); }
  const_reverse_iterator rend() const noexcept { return exprs_.crend(); }

  const_reverse_iterator crbegin() const noexcept { return exprs_.crbegin(); }

  const_reverse_iterator crend() const noexcept { return exprs_.crend(); }

  template <class Op2, size_t N2>
  friend bool operator==(op_base<Op2, N2> const& lhs, op_base<Op2, N2> const& rhs);
};
template <class Op, size_t N>
bool operator==(op_base<Op, N> const& lhs, op_base<Op, N> const& rhs);

};  // namespace ast
};  // namespace scopion

#endif
