#pragma once

#include <QString>
#include <vector>

#include "presentation/resources/ViewModels.h"

namespace presentation {

class IDicomViewerView {
 public:
  virtual ~IDicomViewerView() = default;

  virtual void ShowErrorMessage(const QString& message) = 0;
  virtual void ShowStatusBarMessage(const QString& message, int durationMs = 0) = 0;

  virtual void SetMetaInfo(const QString& text) = 0;

  virtual void PopulateWindowPresets(
    const std::vector<ViewPresetItem>& presets,
    int activeIndex,
    bool enable
  ) = 0;
  virtual void SelectWindowPreset(int index) = 0;
  virtual void UpdateCustomPresetText(const QString& text) = 0;


  virtual void SetFitActionEnabled(bool enabled) = 0;
  virtual void SetMeasureActionsEnabled(bool enabled) = 0;

  virtual void SetZoomPercent(int percent) = 0;
  virtual void SetStatusBarWl(const QString& text) = 0;
  virtual void SetStatusBarDimensions(const QString& text) = 0;
};

}  // namespace presentation
