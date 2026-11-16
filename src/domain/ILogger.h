#pragma once

#include <cstdint>
#include <string>

namespace domain {

enum class LogLevel : std::uint8_t {
  kTrace,
  kDebug,
  kInfo,
  kWarn,
  kError,
  kCritical,
};

class ILogger {
 public:
  virtual ~ILogger() = default;

  virtual void Log(LogLevel level, const std::string& message) = 0;

  void Trace(const std::string& message) {
    Log(LogLevel::kTrace, message);
  }

  void Debug(const std::string& message) {
    Log(LogLevel::kDebug, message);
  }

  void Info(const std::string& message) {
    Log(LogLevel::kInfo, message);
  }

  void Warn(const std::string& message) {
    Log(LogLevel::kWarn, message);
  }

  void Error(const std::string& message) {
    Log(LogLevel::kError, message);
  }

  void Critical(const std::string& message) {
    Log(LogLevel::kCritical, message);
  }
};

class NullLogger final : public ILogger {
 public:
  void Log(LogLevel /*level*/, const std::string& /*message*/) override {}
};

}  // namespace domain