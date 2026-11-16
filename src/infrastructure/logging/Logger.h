#pragma once

#include <cstddef>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "domain/ILogger.h"

namespace infrastructure {

inline std::string Redact(std::string_view value) {
  if (value.empty()) return "(empty)";
  if (value.size() == 1) return "*";
  if (value.size() == 2) return std::string(1, value[0]) + "*";
  return std::string(value.substr(0, 2)) + std::string(value.size() - 2, '*');
}

class Logger {
 public:
  struct Config {
    std::string logFile;
    domain::LogLevel level = domain::LogLevel::kInfo;
    std::size_t maxBytes = 10 * 1024 * 1024;
  };

  static void Initialize(const Config& config);
  static void Shutdown() noexcept;
  static void Flush() noexcept;

  template <typename... Args>
  static void Trace(std::format_string<Args...> fmt, Args&&... args) {
    Write(domain::LogLevel::kTrace, std::format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args>
  static void Debug(std::format_string<Args...> fmt, Args&&... args) {
    Write(domain::LogLevel::kDebug, std::format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args>
  static void Info(std::format_string<Args...> fmt, Args&&... args) {
    Write(domain::LogLevel::kInfo, std::format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args>
  static void Warn(std::format_string<Args...> fmt, Args&&... args) {
    Write(domain::LogLevel::kWarn, std::format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args>
  static void Error(std::format_string<Args...> fmt, Args&&... args) {
    Write(domain::LogLevel::kError, std::format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args>
  static void Critical(std::format_string<Args...> fmt, Args&&... args) {
    Write(domain::LogLevel::kCritical, std::format(fmt, std::forward<Args>(args)...));
  }

 private:
  static void Write(domain::LogLevel newLevel, const std::string& message);
  static const char* LevelName(domain::LogLevel newLevel);

  static std::mutex mutex;
  static std::optional<std::ofstream> file;
  static domain::LogLevel level;
  static bool initialized;
};

}  // namespace infrastructure