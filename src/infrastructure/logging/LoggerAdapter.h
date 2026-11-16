#pragma once

#include "domain/ILogger.h"

namespace infrastructure {

class LoggerAdapter final : public domain::ILogger {
 public:
  void Log(
    domain::LogLevel level,
    const std::string& message
  ) override;
};

}  // namespace infrastructure