#ifndef SCOPION_EXCEPTIONS_H_
#define SCOPION_EXCEPTIONS_H_

#include <algorithm>

#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/iterator_range.hpp>

namespace scopion {
using str_range_t = boost::iterator_range<std::string::const_iterator>;

class general_error : public std::runtime_error {

public:
  str_range_t where;
  str_range_t code;
  uint8_t level;
  general_error(std::string const &message, str_range_t where_,
                str_range_t code_, uint8_t level_ = 0)
      : std::runtime_error(message), where(where_), code(code_), level(level_) {
  }
};

class diagnosis {
  general_error const &ex_;

public:
  diagnosis(general_error const &ex) : ex_(ex) {}

  std::string what() { return ex_.what(); }

  uint32_t line_n() {
    return std::count(ex_.code.begin(), ex_.where.begin(), '\n');
  }
  str_range_t line_range() {
    auto range =
        boost::make_iterator_range(ex_.code.begin(), ex_.where.begin());
    auto sol = boost::find(range | boost::adaptors::reversed, '\n').base();
    auto eol = std::find(ex_.where.begin(), ex_.code.end(), '\n');
    return boost::make_iterator_range(sol, eol);
  }
  std::string line() {
    auto r = line_range();
    return std::string(r.begin(), r.end());
  }
  uint16_t line_c() {
    auto r = line_range();
    return std::distance(r.begin(), ex_.where.begin());
  }
};
/*
namespace assembly {
class error : public general_error {};
}; // namespace assembly

namespace parser {
class error : public general_error {};
}; // namespace parser*/

}; // namespace scopion

#endif // SCOPION_EXCEPTIONS_H_
