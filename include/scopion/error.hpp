/**
* @file error.hpp
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

#ifndef SCOPION_ERROR_H_
#define SCOPION_ERROR_H_

#include <algorithm>
#include <iomanip>
#include <iostream>

#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/iterator_range.hpp>

#include "rang.hpp"

namespace scopion
{
using str_range_t = boost::iterator_range<std::string::const_iterator>;

class error
{
  static str_range_t line_range(str_range_t const where, str_range_t const code)
  {
    auto range = boost::make_iterator_range(code.begin(), where.begin());
    auto sol   = boost::find(range | boost::adaptors::reversed, '\n').base();
    auto eol   = std::find(where.begin(), code.end(), '\n');
    return boost::make_iterator_range(sol, eol);
  }
  uint32_t line_number_;
  uint32_t distance_;
  std::string line_;
  std::string message_;
  uint8_t level_;

public:
  error(std::string const& message,
        str_range_t const where,
        str_range_t const code,
        uint8_t level = 0)
      : message_(message), level_(level)
  {
    line_number_ = static_cast<uint32_t>(std::count(code.begin(), where.begin(), '\n')) + 1;
    auto r       = line_range(where, code);
    line_        = std::string(r.begin(), r.end());
    distance_    = static_cast<uint32_t>(std::distance(r.begin(), where.begin()));
  }

  uint32_t line_number() const { return line_number_; }
  std::string line() const { return line_; }
  uint32_t distance() const { return distance_; }
  uint8_t level() const { return level_; }
  std::string what() const { return message_; }
};

template <class Char, class Traits>
std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& os, const error& e)
{
  return os << rang::style::reset << rang::bg::red << rang::fg::gray << "[ERROR]"
            << rang::style::reset << rang::fg::red << " @" << e.line_number() << rang::style::reset
            << ": " << e.what() << std::endl
            << e.line() << std::endl
            << rang::fg::green << std::setw(static_cast<int>(e.distance()) + 1) << "^"
            << rang::style::reset;
}

};  // namespace scopion

#endif  // SCOPION_EXCEPTIONS_H_
