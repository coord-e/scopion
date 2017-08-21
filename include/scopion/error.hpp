#ifndef SCOPION_ERROR_H_
#define SCOPION_ERROR_H_

#include <algorithm>

#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/iterator_range.hpp>

namespace scopion
{
class error
{
  using str_range_t = boost::iterator_range<std::string::const_iterator>;

  static str_range_t line_range(str_range_t const where, str_range_t const code)
  {
    auto range = boost::make_iterator_range(code.begin(), where.begin());
    auto sol   = boost::find(range | boost::adaptors::reversed, '\n').base();
    auto eol   = std::find(where.begin(), code.end(), '\n');
    return boost::make_iterator_range(sol, eol);
  }
  uint64_t line_number_;
  uint64_t distance_;
  std::string line_;
  std::string filename_;
  std::string message_;

public:
  error() = default;

  error(std::string const& message,
        str_range_t const where,
        str_range_t const code,
        std::string const& filename = "")
      : message_(message), filename_(filename)
  {
    line_number_ = std::count(code.begin(), where.begin(), '\n');
    auto r       = line_range(where, code);
    line_        = std::string(r.begin(), r.end());
    distance_    = std::distance(r.begin(), where.begin());
  }

  uint64_t getLineNumber() const { return line_number_; }
  std::string getLineContent() const { return line_; }
  void setFilename(std::string const& s) { filename_ = s; }
  std::string getFilename() const { return filename_; }
  std::string getMessage() const { return message_; }
  uint64_t getColumnNumber() const { return distance_; }
};

};  // namespace scopion

#endif  // SCOPION_EXCEPTIONS_H_
