#include "presentation/ui/MainWindow.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPalette>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>

#include "presentation/widgets/DicomViewerWidget.h"
#include "presentation/ui/RibbonBar.h"
#include "presentation/resources/UiStrings.h"
#include "infrastructure/GdcmDicomLoader.h"
#include "infrastructure/logging/LoggerAdapter.h"

namespace presentation {

MainWindow::MainWindow(QWidget* newParent)
  : QMainWindow(newParent)
{
  BuildUi();
  BuildRibbon();
  BuildStatusBar();
}

MainWindow::~MainWindow() = default;

void MainWindow::BuildUi() {
  QMenu* fileMenu = menuBar()->addMenu(UiStrings::MenuFile());
  QAction* openAction = new QAction(UiStrings::MenuOpen(), this);
  openAction->setShortcut(QKeySequence::Open);
  connect(openAction, &QAction::triggered, this, &MainWindow::OpenFile);
  fileMenu->addAction(openAction);

  QAction* closeAction = new QAction(UiStrings::MenuClose(), this);
  closeAction->setShortcut(QKeySequence("Ctrl+W"));
  connect(closeAction, &QAction::triggered, this, &MainWindow::CloseCurrentFile);
  fileMenu->addAction(closeAction);

  QAction* closeAllAction = new QAction(UiStrings::MenuCloseAll(), this);
  closeAllAction->setShortcut(QKeySequence("Ctrl+Shift+W"));
  connect(closeAllAction, &QAction::triggered, this, &MainWindow::CloseAllFiles);
  fileMenu->addAction(closeAllAction);

  fileMenu->addSeparator();

  QAction* quitAction = new QAction(UiStrings::MenuQuit(), this);
  quitAction->setShortcut(QKeySequence::Quit);
  connect(quitAction, &QAction::triggered, this, &QWidget::close);
  fileMenu->addAction(quitAction);
}

void MainWindow::BuildRibbon() {
  m_ribbon = new RibbonBar(this);

  QWidget* central = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(central);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  layout->addWidget(m_ribbon);

  QWidget* contentContainer = new QWidget(this);
  QVBoxLayout* contentLayout = new QVBoxLayout(contentContainer);
  contentLayout->setContentsMargins(0, 0, 0, 0);
  contentLayout->setSpacing(0);

  m_emptyPlaceholder = BuildEmptyPlaceholder();

  m_fileTabs = new QTabWidget(this);
  m_fileTabs->setTabsClosable(true);
  m_fileTabs->setMovable(true);
  m_fileTabs->setDocumentMode(true);
  {
    QPalette tabPal = m_fileTabs->palette();
    tabPal.setColor(QPalette::Window, QColor(0x1e, 0x1e, 0x1e));
    tabPal.setColor(QPalette::WindowText, QColor(0xcc, 0xcc, 0xcc));
    tabPal.setColor(QPalette::BrightText, QColor(0xff, 0xff, 0xff));
    m_fileTabs->setPalette(tabPal);
  }
  connect(m_fileTabs, &QTabWidget::tabCloseRequested, this, &MainWindow::OnTabCloseRequested);
  connect(m_fileTabs, &QTabWidget::currentChanged, this, &MainWindow::OnTabChanged);

  contentLayout->addWidget(m_emptyPlaceholder);
  contentLayout->addWidget(m_fileTabs);
  UpdateContentVisibility();

  layout->addWidget(contentContainer, 1);

  setCentralWidget(central);
  resize(1280, 900);
  setWindowTitle(UiStrings::AppTitle());

  ConnectRibbonToActiveTab();
}

QWidget* MainWindow::BuildEmptyPlaceholder() {
  QWidget* widget = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(widget);
  layout->setAlignment(Qt::AlignCenter);

  QLabel* icon = new QLabel("📄", widget);
  {
    QFont font = QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont);
    font.setPointSize(48);
    icon->setFont(font);
    QPalette pal = icon->palette();
    pal.setColor(QPalette::WindowText, QColor(0x66, 0x66, 0x66));
    icon->setPalette(pal);
  }
  layout->addWidget(icon, 0, Qt::AlignCenter);

  QLabel* text = new QLabel(UiStrings::NoFilesOpen(), widget);
  {
    QFont font;
    font.setPointSize(16);
    text->setFont(font);
    QPalette pal = text->palette();
    pal.setColor(QPalette::WindowText, QColor(0x88, 0x88, 0x88));
    text->setPalette(pal);
  }
  text->setAlignment(Qt::AlignCenter);
  layout->addWidget(text, 0, Qt::AlignCenter);

  QLabel* hint = new QLabel(UiStrings::NoFilesOpenHint(), widget);
  {
    QFont font;
    font.setPointSize(12);
    hint->setFont(font);
    QPalette pal = hint->palette();
    pal.setColor(QPalette::WindowText, QColor(0x55, 0x55, 0x55));
    hint->setPalette(pal);
  }
  hint->setAlignment(Qt::AlignCenter);
  layout->addWidget(hint, 0, Qt::AlignCenter);

  widget->setLayout(layout);
  return widget;
}

void MainWindow::ConnectRibbonToActiveTab() {
  connect(m_ribbon, &RibbonBar::OpenFileRequested, this, &MainWindow::OpenFile);
  connect(m_ribbon, &RibbonBar::CloseFileRequested, this, &MainWindow::CloseCurrentFile);
  connect(m_ribbon, &RibbonBar::CloseAllFilesRequested, this, &MainWindow::CloseAllFiles);
  connect(m_ribbon, &RibbonBar::WindowPresetChanged, this, &MainWindow::OnWindowPresetChanged);

  ConnectNavigationActions();
  ConnectMeasureActions();
  ConnectTransformActions();

  auto safePresenter = [this]() -> DicomViewerPresenter* {
    FileTab* tab = ActiveTab();
    return tab ? tab->presenter.get() : nullptr;
  };
  connect(m_ribbon, &RibbonBar::WindowLevelToggled, this, [safePresenter](bool checked) {
    if (auto* p = safePresenter()) {
      if (checked) p->SetWindowLevelTool();
      else p->SetPanTool();
    }
  });
}

void MainWindow::ConnectNavigationActions() {
  auto safePresenter = [this]() -> DicomViewerPresenter* {
    FileTab* tab = ActiveTab();
    return tab ? tab->presenter.get() : nullptr;
  };
  auto safeViewer = [this]() -> DicomViewerWidget* {
    FileTab* tab = ActiveTab();
    return tab ? tab->viewer : nullptr;
  };

  connect(m_ribbon, &RibbonBar::FitRequested, this, [safePresenter]() {
    if (auto* p = safePresenter()) p->OnFitRequested();
  });
  connect(m_ribbon, &RibbonBar::Zoom11Requested, this, [safeViewer]() {
    if (auto* v = safeViewer()) v->ZoomToOneToOne();
  });
  connect(m_ribbon, &RibbonBar::FitContentRequested, this, [safeViewer]() {
    if (auto* v = safeViewer()) v->FitToImageContent();
  });
}

void MainWindow::ConnectMeasureActions() {
  auto safePresenter = [this]() -> DicomViewerPresenter* {
    FileTab* tab = ActiveTab();
    return tab ? tab->presenter.get() : nullptr;
  };

  connect(m_ribbon, &RibbonBar::MeasureToggled, this, [safePresenter](bool c) {
    if (auto* p = safePresenter()) p->OnToolToggled(ViewerTool::kMeasure, c);
  });
  connect(m_ribbon, &RibbonBar::AngleToggled, this, [safePresenter](bool c) {
    if (auto* p = safePresenter()) p->OnToolToggled(ViewerTool::kAngle, c);
  });
  connect(m_ribbon, &RibbonBar::CircleRoiToggled, this, [safePresenter](bool c) {
    if (auto* p = safePresenter()) p->OnToolToggled(ViewerTool::kCircleRoi, c);
  });
  connect(m_ribbon, &RibbonBar::RectRoiToggled, this, [safePresenter](bool c) {
    if (auto* p = safePresenter()) p->OnToolToggled(ViewerTool::kRectRoi, c);
  });
  connect(m_ribbon, &RibbonBar::ClearMeasurementsRequested, this, [safePresenter]() {
    if (auto* p = safePresenter()) p->OnClearMeasurementsRequested();
  });
}

void MainWindow::ConnectTransformActions() {
  auto safeViewer = [this]() -> DicomViewerWidget* {
    FileTab* tab = ActiveTab();
    return tab ? tab->viewer : nullptr;
  };

  connect(m_ribbon, &RibbonBar::RotateCwRequested, this, [safeViewer]() {
    if (auto* v = safeViewer()) v->RotateClockwise();
  });
  connect(m_ribbon, &RibbonBar::RotateCcwRequested, this, [safeViewer]() {
    if (auto* v = safeViewer()) v->RotateCounterClockwise();
  });
  connect(m_ribbon, &RibbonBar::FlipHRequested, this, [safeViewer]() {
    if (auto* v = safeViewer()) v->FlipHorizontal();
  });
  connect(m_ribbon, &RibbonBar::FlipVRequested, this, [safeViewer]() {
    if (auto* v = safeViewer()) v->FlipVertical();
  });
}

void MainWindow::BuildStatusBar() {
  QStatusBar* bar = statusBar();

  zoomLabel = CreateStatusLabel(UiStrings::ZoomInitial(), 60);
  wlStatusLabel = CreateStatusLabel("", 120);
  dimLabel = CreateStatusLabel("", 100);
  QLabel* cursorLabel = CreateStatusLabel("", 180);

  bar->addWidget(zoomLabel, 1);
  bar->addWidget(wlStatusLabel, 1);
  bar->addWidget(dimLabel, 1);
  bar->addPermanentWidget(cursorLabel);
}

QLabel* MainWindow::CreateStatusLabel(const QString& text, int minWidth) {
  QLabel* label = new QLabel(text, this);
  label->setMinimumWidth(minWidth);
  label->setPalette(MakeInfoLabelPalette());
  return label;
}

FileTab* MainWindow::ActiveTab() {
  if (m_fileTabsList.empty()) return nullptr;
  const int index = m_fileTabs->currentIndex();
  if (index < 0 || index >= static_cast<int>(m_fileTabsList.size())) return nullptr;
  return &m_fileTabsList[static_cast<std::size_t>(index)];
}

FileTab* MainWindow::AddFileTab(std::unique_ptr<DicomViewerPresenter> presenter, const QString& fileName) {
  DicomViewerWidget* viewer = new DicomViewerWidget(this);
  DicomViewerPresenter* p = presenter.get();
  ConnectViewerSignals(viewer, p);

  QLabel* metaLabel = CreateMetaLabel();

  QWidget* container = new QWidget(this);
  QVBoxLayout* containerLayout = new QVBoxLayout(container);
  containerLayout->setContentsMargins(0, 0, 0, 0);
  containerLayout->setSpacing(0);
  containerLayout->addWidget(metaLabel);
  containerLayout->addWidget(viewer, 1);

  presenter->SetView(this);
  presenter->SetViewerWidget(viewer);

  const int tabIndex = m_fileTabs->addTab(container, fileName);

  FileTab tab;
  tab.presenter = std::move(presenter);
  tab.viewer = viewer;
  tab.metaLabel = metaLabel;
  tab.fileName = fileName;
  tab.container = container;
  m_fileTabsList.push_back(std::move(tab));

  UpdateContentVisibility();
  m_fileTabs->setCurrentIndex(tabIndex);
  return &m_fileTabsList.back();
}

void MainWindow::ConnectViewerSignals(DicomViewerWidget* viewer, DicomViewerPresenter* presenter) {
  connect(viewer, &DicomViewerWidget::CursorPositionChanged,
          this, [presenter](int x, int y, int value) {
            presenter->OnCursorPositionChanged(x, y, value);
          });
  connect(viewer, &DicomViewerWidget::WindowLevelChanged,
          this, [presenter](double center, double width) {
            presenter->OnWindowLevelChanged(center, width);
          });
  connect(viewer, &DicomViewerWidget::ZoomChanged,
          this, [this](int percent) {
            SetZoomPercent(percent);
          });
}

QLabel* MainWindow::CreateMetaLabel() {
  QLabel* label = new QLabel(UiStrings::NoExamLoaded(), this);
  label->setAutoFillBackground(true);
  label->setPalette(MakeMetaLabelPalette());
  label->setContentsMargins(8, 4, 8, 4);
  label->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  label->setWordWrap(true);
  return label;
}

void MainWindow::RemoveFileTab(int index) {
  if (index < 0 || index >= static_cast<int>(m_fileTabsList.size())) return;

  m_fileTabsList.erase(m_fileTabsList.begin() + static_cast<std::size_t>(index));

  m_fileTabs->removeTab(index);

  UpdateContentVisibility();
}

void MainWindow::SwitchToTab(int index) {
  if (index < 0 || index >= static_cast<int>(m_fileTabsList.size())) return;
  m_fileTabs->setCurrentIndex(index);
}

void MainWindow::OnTabChanged(int /*index*/) {
  // Os connects do ribbon usam lambdas que consultam ActiveTab()
}

void MainWindow::UpdateContentVisibility() {
  const bool hasFiles = !m_fileTabsList.empty();
  if (m_emptyPlaceholder) m_emptyPlaceholder->setVisible(!hasFiles);
  if (m_fileTabs) m_fileTabs->setVisible(hasFiles);
}

void MainWindow::OnTabCloseRequested(int index) {
  RemoveFileTab(index);
}

void MainWindow::OpenFile() {
  const QString path = QFileDialog::getOpenFileName(
    this,
    UiStrings::DialogOpenTitle(),
    {},
    UiStrings::DialogOpenFilter()
  );

  if (path.isEmpty()) return;

  const QString fileName = QFileInfo(path).fileName();

  auto loggerAdapter = std::make_unique<infrastructure::LoggerAdapter>();
  auto presenter = std::make_unique<DicomViewerPresenter>(
    std::make_unique<infrastructure::GdcmDicomLoader>(),
    *loggerAdapter
  );

  FileTab* tab = AddFileTab(std::move(presenter), fileName);
  if (tab) tab->loggerAdapter = std::move(loggerAdapter);

  tab->presenter->OnFileSelected(path.toStdString());
}

void MainWindow::CloseCurrentFile() {
  const int index = m_fileTabs->currentIndex();
  if (index >= 0) {
    RemoveFileTab(index);
  }
}

void MainWindow::CloseAllFiles() {
  while (!m_fileTabsList.empty()) {
    RemoveFileTab(0);
  }
}

void MainWindow::ShowErrorMessage(const QString& message) {
  QMessageBox::critical(this, UiStrings::ErrLoadDicomTitle(), message);
}

void MainWindow::ShowStatusBarMessage(const QString& message, int durationMs) {
  statusBar()->showMessage(message, durationMs);
}

void MainWindow::SetMetaInfo(const QString& text) {
  FileTab* tab = ActiveTab();
  if (tab && tab->metaLabel) {
    tab->metaLabel->setText(text);
  }
}

void MainWindow::PopulateWindowPresets(
  const std::vector<ViewPresetItem>& presets,
  int activeIndex,
  bool enable
) {
  if (!m_ribbon) return;

  QStringList items;
  items.reserve(static_cast<qsizetype>(presets.size()));
  for (const ViewPresetItem& item : presets) {
    items.append(item.text);
  }
  m_ribbon->PopulateWindowPresets(items, activeIndex, enable);
}

void MainWindow::SelectWindowPreset(int index) {
  if (m_ribbon) m_ribbon->SelectWindowPreset(index);
}

void MainWindow::UpdateCustomPresetText(const QString& text) {
  if (m_ribbon) m_ribbon->UpdateCustomPresetText(text);
}

void MainWindow::SetFitActionEnabled(bool enabled) {
  if (m_ribbon) m_ribbon->SetNavigationActionsEnabled(enabled);
}

void MainWindow::SetMeasureActionsEnabled(bool enabled) {
  if (m_ribbon) m_ribbon->SetMeasurementActionsEnabled(enabled);
}

void MainWindow::SetZoomPercent(int percent) {
  if (zoomLabel) {
    zoomLabel->setText(QString::number(percent) + "%");
  }
}

void MainWindow::SetStatusBarWl(const QString& text) {
  if (wlStatusLabel) {
    wlStatusLabel->setText(text);
  }
}

void MainWindow::SetStatusBarDimensions(const QString& text) {
  if (dimLabel) {
    dimLabel->setText(text);
  }
}

void MainWindow::OnWindowPresetChanged(int newIndex) {
  FileTab* tab = ActiveTab();
  if (tab && newIndex >= 0) {
    tab->presenter->OnWindowPresetChanged(newIndex);
  }
}

QPalette MainWindow::MakeMetaLabelPalette() {
  QPalette palette;
  palette.setColor(QPalette::Window, QColor(0x20, 0x20, 0x20));
  palette.setColor(QPalette::WindowText, QColor(0xdd, 0xdd, 0xdd));
  return palette;
}

QPalette MainWindow::MakeInfoLabelPalette() {
  QPalette palette;
  palette.setColor(QPalette::WindowText, QColor(0x99, 0x99, 0x99));
  return palette;
}

QPalette MainWindow::MakeHintLabelPalette() {
  QPalette palette;
  palette.setColor(QPalette::WindowText, QColor(0x66, 0x66, 0x66));
  return palette;
}

}  // namespace presentation
