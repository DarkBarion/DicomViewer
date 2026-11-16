#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "domain/IDicomLoader.h"
#include "domain/ILogger.h"
#include "domain/DicomImageData.h"
#include "domain/DicomMetadata.h"
#include "domain/Measurement.h"

#include "presentation/resources/ViewModels.h"
#include "presentation/presenter/IDicomViewerView.h"
#include "presentation/presenter/IDicomViewerWidget.h"

namespace presentation {

class DicomViewerPresenter {
 public:
  DicomViewerPresenter(
    std::unique_ptr<domain::IDicomLoader> loader,
    domain::ILogger& logger
  );

  void SetView(IDicomViewerView* newView) {
    view = newView;
  }

  void SetViewerWidget(IDicomViewerWidget* newWidget) {
    viewerWidget = newWidget;
  }

  void OnFileSelected(std::string_view path);
  void OnWindowPresetChanged(int index);
  void OnWindowLevelChanged(double center, double width);
  void OnFitRequested();
  void OnToolToggled(ViewerTool tool, bool checked);
  void OnClearMeasurementsRequested();
  void OnCursorPositionChanged(int x, int y, int value);

  void SetWindowLevelTool();
  void SetPanTool();

  IDicomViewerWidget* GetViewerWidget() const {
    return viewerWidget;
  }

 private:
  void LoadFile(std::string_view path);
  void ShowLoadedImage();
  void ShowImageWarnings();
  void UpdateCustomPresetItem();
  void UpdateOverlayInfo();
  void PopulateWindowPresets();

  void ApplyCustomPreset();
  void ApplyVoiLutPreset();
  void ApplyWindowPreset(int windowLevelIndex);

  domain::WindowLevel GetInitialWindowLevel(const domain::DicomImageData& imageData);
  QString FormatPresetText(const domain::WindowLevel& preset);
  QString FormatCustomText(const domain::WindowLevel& windowLevel);
  std::vector<ViewPresetItem> BuildPresetList(const domain::DicomImageData& imageData);
  int ComputeActivePresetIndex(const domain::DicomImageData& imageData, int presetCount);

  QStringList BuildMetadataParts(const domain::DicomMetadata& metadata);
  QString FormatImageDimensions(const domain::DicomImageData& image);
  QString FormatPixelSpacing(const domain::DicomImageData& image);
  QString FormatWindowLevelStatus(const domain::DicomImageData& image);

  std::unique_ptr<domain::IDicomLoader> loader;
  domain::ILogger& logger;
  std::optional<domain::LoadedDicom> current;

  IDicomViewerView* view = nullptr;
  IDicomViewerWidget* viewerWidget = nullptr;

  bool updatingPresetCombo = false;
};

}  // namespace presentation
