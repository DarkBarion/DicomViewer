#pragma once

#include <memory>

#include "ApplicationBootstrap.h"

class QApplication;
class QTranslator;

namespace presentation {
class MainWindow;
}  // namespace presentation

namespace app {

enum class ExitCode : int {
  kSuccess = 0,
  kInvalidArguments = 1,
  kInitializationFailed = 2,
  kRuntimeError = 3,
  kUnexpectedError = 4,
};

class Application {
 public:
  Application(
    int& argc,
    char** argv,
    std::unique_ptr<ApplicationBootstrap> newBootstrap = nullptr
  );

  ~Application();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  Application(Application&&) = default;
  Application& operator=(Application&&) = default;

  bool Initialize();
  int Run();
  void Shutdown() noexcept;

 private:
  std::unique_ptr<QApplication> qtApp;
  std::unique_ptr<QTranslator> translator;
  std::unique_ptr<ApplicationBootstrap> bootstrap;
  std::unique_ptr<presentation::MainWindow> mainWindow;
  bool initialized = false;
};

}  // namespace app