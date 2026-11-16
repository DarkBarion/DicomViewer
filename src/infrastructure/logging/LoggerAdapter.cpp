#include "LoggerAdapter.h"

#include "Logger.h"

namespace infrastructure {

void LoggerAdapter::Log(
  const domain::LogLevel level,
  const std::string& message
)
{
  switch (level) {
    case domain::LogLevel::kTrace:    Logger::Trace("{}", message);    break;
    case domain::LogLevel::kDebug:    Logger::Debug("{}", message);    break;
    case domain::LogLevel::kInfo:     Logger::Info("{}", message);     break;
    case domain::LogLevel::kWarn:     Logger::Warn("{}", message);     break;
    case domain::LogLevel::kError:    Logger::Error("{}", message);    break;
    case domain::LogLevel::kCritical: Logger::Critical("{}", message); break;
  }
}

}  // namespace infrastructure