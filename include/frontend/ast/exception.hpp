#pragma once

#include <stdexcept>
namespace frontend {
class Exception : public std::runtime_error {
  public:
    explicit Exception(const std::string &s) : std::runtime_error(s) {}
};

class InvalidOperation : public Exception {
  public:
    using Exception::Exception;
};

}; // namespace frontend
