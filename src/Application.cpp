#include "Application.h"

#include <memory>

#include <QApplication>
#include <QDir>
#include <QLocale>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTranslator>

#include "infrastructure/logging/Logger.h"
#include "presentation/MainWindow.h"

namespace app {

Application::Application(
  int& argc,
  char** argv,
  std::unique_ptr<ApplicationBootstrap> newBootstrap
) :
  qtApp(std::make_unique<QApplication>(argc, argv)),
  bootstrap(std::move(newBootstrap))
{
  QApplication::setApplicationName("DicomViewer");
  QApplication::setApplicationVersion("0.1.0");
  QApplication::setApplicationDisplayName("Dicom Viewer");
}

Application::~Application() {
  Shutdown();
}

bool Application::Initialize() {
  if (initialized) {
    return true;
  }

  const QString logDirectory = QStandardPaths::writableLocation(
    QStandardPaths::AppLocalDataLocation
  );
  QDir().mkpath(logDirectory);
  const QString logPath = logDirectory + "/dicom_viewer.log";

  infrastructure::Logger::Config config;
  config.logFile = logPath.toStdString();
  config.level = domain::LogLevel::kInfo;
  config.maxBytes = 10u * 1024u * 1024u;
  infrastructure::Logger::Initialize(config);
  infrastructure::Logger::Info("Log file: {}", logPath.toStdString());

  QLocale::setDefault(QLocale(QLocale::Portuguese, QLocale::Brazil));

  translator = std::make_unique<QTranslator>(qtApp.get());
  const QLocale locale = QLocale::system();
  if (translator->load(locale, "DicomViewer", "_", ":/i18n")) {
    qtApp->installTranslator(translator.get());
    infrastructure::Logger::Info(
      "Translation loaded: {}", locale.name().toStdString()
    );
  }
  else {
    infrastructure::Logger::Info(
      "No translation for locale, using source strings"
    );
    translator.reset();
  }

  try {
    if (!bootstrap) {
      bootstrap = std::make_unique<ApplicationBootstrap>();
    }

    domain::Status status = bootstrap->InitializeServices();
    if (!status) {
      infrastructure::Logger::Critical(
        "Init failed: {}", status.error
      );
      QMessageBox::critical(
        nullptr,
        QObject::tr("Erro ao iniciar"),
        QObject::tr("Não foi possível iniciar a aplicação:\n%1")
          .arg(QString::fromStdString(status.error))
      );
      return false;
    }

    mainWindow = bootstrap->CreateMainWindow();
    if (!mainWindow) {
      infrastructure::Logger::Critical(
        "CreateMainWindow returned null"
      );
      return false;
    }

    mainWindow->show();
    initialized = true;
    infrastructure::Logger::Info("Application initialized");
    return true;
  }
  catch (const std::exception& exception) {
    infrastructure::Logger::Critical(
      "Exception in Initialize(): {}", exception.what()
    );
    return false;
  }
  catch (...) {
    infrastructure::Logger::Critical(
      "Unknown exception in Initialize()"
    );
    return false;
  }
}

int Application::Run() {
  if (!Initialize()) {
    return std::to_underlying(ExitCode::kInitializationFailed);
  }

  int code = std::to_underlying(ExitCode::kSuccess);
  try {
    code = QApplication::exec();
  }
  catch (const std::exception& exception) {
    infrastructure::Logger::Critical(
      "Unhandled exception in exec(): {}", exception.what()
    );
    code = std::to_underlying(ExitCode::kUnexpectedError);
  }
  catch (...) {
    infrastructure::Logger::Critical(
      "Unknown exception in exec()"
    );
    code = std::to_underlying(ExitCode::kUnexpectedError);
  }

  Shutdown();
  return code;
}

void Application::Shutdown() noexcept {
  if (!initialized) {
    return;
  }

  try {
    if (bootstrap) bootstrap->ShutdownServices();
  }
  catch (...) {
    infrastructure::Logger::Error("Exception during ShutdownServices()");
  }

  try {
    infrastructure::Logger::Flush();
    infrastructure::Logger::Shutdown();
  }
  catch (...) {
    // Logger pode estar danificado durante shutdown; nada seguro a fazer.
    // Nao ha como logar, pois o proprio Logger falhou.
  }

  initialized = false;
}

}  // namespace app