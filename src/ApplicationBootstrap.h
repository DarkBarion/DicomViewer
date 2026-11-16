#pragma once

#include <memory>

#include "domain/Result.h"

namespace domain {
class IDicomLoader;
class ILogger;
}  // namespace domain

namespace presentation {
class MainWindow;
class DicomViewerPresenter;
}  // namespace presentation

namespace app {

class ApplicationBootstrap {
 public:
  ApplicationBootstrap();
  ~ApplicationBootstrap();

  ApplicationBootstrap(const ApplicationBootstrap&) = delete;
  ApplicationBootstrap& operator=(const ApplicationBootstrap&) = delete;

  ApplicationBootstrap(ApplicationBootstrap&&) = default;
  ApplicationBootstrap& operator=(ApplicationBootstrap&&) = default;

  domain::Status InitializeServices();
  std::unique_ptr<presentation::MainWindow> CreateMainWindow();
  void ShutdownServices() noexcept;

 private:
  std::unique_ptr<domain::IDicomLoader> loader;
  std::unique_ptr<domain::ILogger> logger;
  std::unique_ptr<presentation::DicomViewerPresenter> presenter;
};

}  // namespace app