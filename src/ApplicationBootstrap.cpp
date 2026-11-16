#include "ApplicationBootstrap.h"

#include <exception>
#include <memory>

#include "domain/IDicomLoader.h"
#include "domain/ILogger.h"
#include "infrastructure/GdcmDicomLoader.h"
#include "infrastructure/logging/LoggerAdapter.h"
#include "presentation/MainWindow.h"
#include "presentation/DicomViewerPresenter.h"

namespace app {

ApplicationBootstrap::ApplicationBootstrap() = default;
ApplicationBootstrap::~ApplicationBootstrap() = default;

domain::Status ApplicationBootstrap::InitializeServices() {
  try {
    logger = std::make_unique<infrastructure::LoggerAdapter>();
    loader = std::make_unique<infrastructure::GdcmDicomLoader>();
    presenter = std::make_unique<presentation::DicomViewerPresenter>(
      std::move(loader),
      *logger
    );
    return domain::Status::Success();
  }
  catch (const std::exception& exception) {
    return domain::Status::Fail(exception.what());
  }
}

std::unique_ptr<presentation::MainWindow>
ApplicationBootstrap::CreateMainWindow() {
  return std::make_unique<presentation::MainWindow>(
    std::move(presenter)
  );
}

void ApplicationBootstrap::ShutdownServices() noexcept {
  presenter.reset();
  loader.reset();
}

}  // namespace app