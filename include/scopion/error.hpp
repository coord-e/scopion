#ifndef SCOPION_ERROR_H_
#define SCOPION_ERROR_H_

#include <algorithm>

#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/iterator_range.hpp>

namespace scopion {

class error : public std::runtime_error {
  using str_range_t = boost::iterator_range<std::string::const_iterator>;

public:
  const str_range_t where;
  const str_range_t code;
  const uint8_t level;

  error(std::string const &message, str_range_t const &where_,
        str_range_t const &code_, uint8_t level_ = 0)
      : std::runtime_error(message), where(where_), code(code_), level(level_) {
  }

  uint32_t line_number() const {
    return std::count(code.begin(), where.begin(), '\n');
  }
  str_range_t line_range() const {
    auto range = boost::make_iterator_range(code.begin(), where.begin());
    auto sol = boost::find(range | boost::adaptors::reversed, '\n').base();
    auto eol = std::find(where.begin(), code.end(), '\n');
    return boost::make_iterator_range(sol, eol);
  }
  std::string line() const {
    auto r = line_range();
    return std::string(r.begin(), r.end());
  }
  uint16_t distance() const {
    auto r = line_range();
    return std::distance(r.begin(), where.begin());
  }
};

}; // namespace scopion

#endif // SCOPION_EXCEPTIONS_H_
