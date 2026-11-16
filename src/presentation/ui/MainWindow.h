#pragma once

#include <QFont>
#include <QIcon>
#include <QMainWindow>
#include <QPalette>
#include <memory>
#include <vector>

#include "presentation/presenter/IDicomViewerView.h"
#include "presentation/presenter/DicomViewerPresenter.h"

class QAction;
class QLabel;
class QTabWidget;

namespace infrastructure {
class LoggerAdapter;
}

namespace presentation {

class DicomViewerWidget;
class RibbonBar;

struct FileTab {
  std::unique_ptr<DicomViewerPresenter> presenter;
  std::unique_ptr<infrastructure::LoggerAdapter> loggerAdapter;
  DicomViewerWidget* viewer = nullptr;
  QLabel* metaLabel = nullptr;
  QString fileName;
  QWidget* container = nullptr;
};

class MainWindow : public QMainWindow, public IDicomViewerView {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* newParent = nullptr);

  ~MainWindow() override;

  void ShowErrorMessage(const QString& message) override;
  void ShowStatusBarMessage(const QString& message, int durationMs = 0) override;
  void SetMetaInfo(const QString& text) override;

  void PopulateWindowPresets(
    const std::vector<ViewPresetItem>& presets,
    int activeIndex,
    bool enable
  ) override;
  void SelectWindowPreset(int index) override;
  void UpdateCustomPresetText(const QString& text) override;


  void SetFitActionEnabled(bool enabled) override;
  void SetMeasureActionsEnabled(bool enabled) override;

  void SetZoomPercent(int percent) override;
  void SetStatusBarWl(const QString& text) override;
  void SetStatusBarDimensions(const QString& text) override;

 private slots:
  void OpenFile();
  void CloseCurrentFile();
  void CloseAllFiles();
  void OnWindowPresetChanged(int newIndex);
  void OnTabCloseRequested(int index);
  void OnTabChanged(int index);

 private:
  void BuildUi();
  void BuildRibbon();
  void BuildStatusBar();
  void SwitchToTab(int index);
  void ConnectRibbonToActiveTab();
  void UpdateContentVisibility();

  QWidget* BuildEmptyPlaceholder();
  QLabel* CreateStatusLabel(const QString& text, int minWidth);
  void ConnectNavigationActions();
  void ConnectMeasureActions();
  void ConnectTransformActions();
  void ConnectViewerSignals(DicomViewerWidget* viewer, DicomViewerPresenter* presenter);
  QLabel* CreateMetaLabel();

  static QPalette MakeMetaLabelPalette();
  static QPalette MakeInfoLabelPalette();
  static QPalette MakeHintLabelPalette();

  FileTab* AddFileTab(std::unique_ptr<DicomViewerPresenter> presenter, const QString& fileName);
  void RemoveFileTab(int index);
  FileTab* ActiveTab();

  QTabWidget* m_fileTabs = nullptr;
  QWidget* m_emptyPlaceholder = nullptr;
  RibbonBar* m_ribbon = nullptr;

  QLabel* zoomLabel = nullptr;
  QLabel* wlStatusLabel = nullptr;
  QLabel* dimLabel = nullptr;

  std::vector<FileTab> m_fileTabsList;
};

}  // namespace presentation