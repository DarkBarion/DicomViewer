#pragma once

#include <expected>
#include <stacktrace>
#include <string>
#include <utility>

namespace domain {

struct Error {
  std::string message;
  std::stacktrace trace;
};

template <typename T>
using Result = std::expected<T, Error>;

template <typename T>
inline Result<T> ResultOk(T value) {
  return Result<T>(std::move(value));
}

template <typename T>
inline Result<T> ResultFail(std::string message) {
  return std::unexpected(Error{.message = std::move(message),
                                .trace = std::stacktrace::current()});
}

struct Status {
  bool ok = true;
  std::string error;

  static Status Success() {
    return {.ok = true, .error = {}};
  }

  static Status Fail(std::string message) {
    return {.ok = false, .error = std::move(message)};
  }

  explicit operator bool() const noexcept {
    return ok;
  }
};

}  // namespace domain