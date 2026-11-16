#include "Logger.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>

namespace infrastructure {

void Logger::Initialize(const Config& config)
{
  std::scoped_lock lock(mutex);

  if (initialized)
    file.reset();

  if (config.maxBytes > 0 && std::filesystem::exists(config.logFile)) {
    const std::uintmax_t size = std::filesystem::file_size(config.logFile);
    if (size >= config.maxBytes) {
      const std::string backup = config.logFile + ".1";
      std::error_code code;
      std::filesystem::remove(backup, code);
      std::filesystem::rename(config.logFile, backup, code);
    }
  }

  file.emplace(config.logFile, std::ios::app);
  if (!file->is_open()) {
    std::fprintf(
      stderr,
      "FATAL: could not open log file: %s\n",
      config.logFile.c_str());
    file.reset();
    level = config.level;
    initialized = false;
    return;
  }

  level = config.level;
  initialized = true;
}
void Logger::Shutdown() noexcept
{
  std::scoped_lock lock(mutex);
  if (file.has_value()) {
    file->flush();
    file.reset();
  }
  initialized = false;
}
void Logger::Flush() noexcept
{
  std::scoped_lock lock(mutex);
  if (file.has_value())
    file->flush();
}
void Logger::Write(domain::LogLevel newLevel, const std::string& message)
{
  std::scoped_lock lock(mutex);
  if (!initialized || newLevel < level)
    return;

  const std::chrono::time_point<std::chrono::system_clock> now =
    std::chrono::system_clock::now();
  const std::time_t time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#ifdef _WIN32
  localtime_s(&tm, &time);
#else
  localtime_r(&time, &tm);
#endif

  std::ostringstream timestamp;
  timestamp << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

  *file << timestamp.str() << " [" << LevelName(newLevel) << "] " << message << '\n';
}

const char* Logger::LevelName(domain::LogLevel newLevel)
{
  switch (newLevel) {
  case domain::LogLevel::kTrace:
    return "TRACE";
  case domain::LogLevel::kDebug:
    return "DEBUG";
  case domain::LogLevel::kInfo:
    return "INFO";
  case domain::LogLevel::kWarn:
    return "WARN";
  case domain::LogLevel::kError:
    return "ERROR";
  case domain::LogLevel::kCritical:
    return "CRITICAL";
  }
  return "?";
}

constinit std::mutex Logger::mutex;

std::optional<std::ofstream> Logger::file;

domain::LogLevel Logger::level = domain::LogLevel::kInfo;

bool Logger::initialized = false;

}  // namespace infrastructure