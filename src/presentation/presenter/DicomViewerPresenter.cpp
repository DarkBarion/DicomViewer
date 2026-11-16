#include "presentation/presenter/DicomViewerPresenter.h"

#include <algorithm>
#include <QString>
#include <QImage>
#include <QObject>

#include "presentation/widgets/ImageConverter.h"
#include "presentation/resources/UiStrings.h"

namespace presentation {

DicomViewerPresenter::DicomViewerPresenter(
  std::unique_ptr<domain::IDicomLoader> newLoader,
  domain::ILogger& newLogger
) :
  loader(std::move(newLoader)),
  logger(newLogger)
{
}

void DicomViewerPresenter::OnFileSelected(std::string_view path) {
  LoadFile(path);
}

void DicomViewerPresenter::LoadFile(std::string_view path) {
  auto result = loader->Load(path);
  if (!result) {
    if (view) {
      view->ShowErrorMessage(QString::fromStdString(result.error().message));
    }
    logger.Warn("Load failed: " + result.error().message);
    return;
  }

  current = std::move(result).value();
  ShowLoadedImage();
}

void DicomViewerPresenter::ShowLoadedImage() {
  if (!current) return;

  domain::WindowLevel initialWl = GetInitialWindowLevel(current->image);
  current->image.appliedWindowLevel = initialWl;
  current->image.voiLutApplied = false;

  int presetIndex = -1;
  for (std::size_t i = 0; i < current->image.availableWindowLevels.size(); ++i) {
    if (current->image.availableWindowLevels[i].center == initialWl.center &&
        current->image.availableWindowLevels[i].width == initialWl.width) {
      presetIndex = static_cast<int>(i);
      break;
    }
  }
  current->image.activeWindowLevelIndex = presetIndex;

  const QImage qimage = ImageConverter::ToQImageWithWindow(current->image, initialWl);
  if (qimage.isNull()) {
    if (view) {
      view->ShowErrorMessage(UiStrings::ErrConvertImage());
    }
    return;
  }

  if (viewerWidget) {
    viewerWidget->DisplayImage(qimage, current->image);
  }

  PopulateWindowPresets();
  UpdateOverlayInfo();
  ShowImageWarnings();
}

void DicomViewerPresenter::ShowImageWarnings() {
  if (!view || !current) return;
  if (current->image.isMonochrome1) {
    view->ShowStatusBarMessage(UiStrings::WarningMonochrome1(), 8000);
  }
  if (current->image.hasOverlays) {
    view->ShowStatusBarMessage(UiStrings::WarningOverlays(), 8000);
  }
}

domain::WindowLevel DicomViewerPresenter::GetInitialWindowLevel(
  const domain::DicomImageData& imageData
) {
  if (imageData.appliedWindowLevel) {
    return imageData.appliedWindowLevel.value();
  }

  if (!imageData.availableWindowLevels.empty()) {
    return imageData.availableWindowLevels.front();
  }

  return domain::WindowLevel::MakeDefault();
}

QString DicomViewerPresenter::FormatPresetText(const domain::WindowLevel& preset) {
  QString text = UiStrings::PresetText(
    QString::fromStdString(preset.label), preset.center, preset.width
  );

  if (preset.function == domain::VoiLutFunction::kSigmoid) {
    text += UiStrings::PresetSigmoidSuffix();
  }
  return text;
}

QString DicomViewerPresenter::FormatCustomText(const domain::WindowLevel& windowLevel) {
  return UiStrings::CustomPresetText(windowLevel.center, windowLevel.width);
}

std::vector<ViewPresetItem> DicomViewerPresenter::BuildPresetList(
  const domain::DicomImageData& imageData
) {
  std::vector<ViewPresetItem> presets;

  if (imageData.hasVoiLut) {
    presets.push_back({UiStrings::VoiLutPreset()});
  }

  for (const domain::WindowLevel& preset : imageData.availableWindowLevels) {
    presets.push_back({FormatPresetText(preset)});
  }

  domain::WindowLevel initial = GetInitialWindowLevel(imageData);
  presets.push_back({FormatCustomText(initial)});

  return presets;
}

int DicomViewerPresenter::ComputeActivePresetIndex(
  const domain::DicomImageData& imageData,
  int presetCount
) {
  int activeIndex = -1;

  if (imageData.activeWindowLevelIndex < 0) {
    activeIndex = imageData.hasVoiLut ? 0 : -1;
  }
  else {
    activeIndex = imageData.activeWindowLevelIndex + (imageData.hasVoiLut ? 1 : 0);
  }

  if (activeIndex < 0 || activeIndex >= presetCount) {
    activeIndex = (presetCount > 0) ? presetCount - 1 : -1;
  }

  return activeIndex;
}

void DicomViewerPresenter::PopulateWindowPresets() {
  if (!view || !current) return;

  const domain::DicomImageData& imageData = current->image;
  std::vector<ViewPresetItem> presets = BuildPresetList(imageData);
  int presetCount = static_cast<int>(presets.size());
  int activeIndex = ComputeActivePresetIndex(imageData, presetCount);

  view->PopulateWindowPresets(presets, activeIndex, !presets.empty());
}

void DicomViewerPresenter::UpdateCustomPresetItem() {
  if (!view || !current) return;

  const domain::WindowLevel* wl = nullptr;
  if (current->image.appliedWindowLevel) {
    wl = &current->image.appliedWindowLevel.value();
  }
  if (!wl) return;

  PopulateWindowPresets();
}

void DicomViewerPresenter::OnWindowPresetChanged(int index) {
  if (!current || index < 0 || updatingPresetCombo) return;

  const bool hasVoiLut = current->image.hasVoiLut;
  const int presetCount = static_cast<int>(current->image.availableWindowLevels.size());
  const int totalItems = (hasVoiLut ? 1 : 0) + presetCount + 1;

  if (index == totalItems - 1) {
    ApplyCustomPreset();
    return;
  }

  if (hasVoiLut && index == 0) {
    ApplyVoiLutPreset();
    return;
  }

  const int windowLevelIndex = hasVoiLut ? index - 1 : index;
  if (windowLevelIndex >= 0 && windowLevelIndex < presetCount) {
    ApplyWindowPreset(windowLevelIndex);
  }
}

void DicomViewerPresenter::ApplyCustomPreset() {
  domain::DicomImageData& imageData = current->image;
  domain::WindowLevel wl;
  if (imageData.appliedWindowLevel) {
    wl = imageData.appliedWindowLevel.value();
  } else {
    wl = GetInitialWindowLevel(imageData);
  }
  if (viewerWidget) {
    viewerWidget->SetWindowLevel(wl);
  }
  imageData.appliedWindowLevel = wl;
  imageData.voiLutApplied = false;
  imageData.activeWindowLevelIndex = -1;
  UpdateOverlayInfo();
}

void DicomViewerPresenter::ApplyVoiLutPreset() {
  domain::DicomImageData& imageData = current->image;
  if (viewerWidget) {
    viewerWidget->RestoreVoiLutPixels();
  }
  imageData.voiLutApplied = true;
  imageData.appliedWindowLevel.reset();
  imageData.activeWindowLevelIndex = -1;
  UpdateOverlayInfo();
}

void DicomViewerPresenter::ApplyWindowPreset(int windowLevelIndex) {
  domain::DicomImageData& imageData = current->image;
  const domain::WindowLevel& preset = imageData.availableWindowLevels[
    static_cast<std::size_t>(windowLevelIndex)
  ];
  if (viewerWidget) {
    viewerWidget->SetWindowLevel(preset);
  }
  imageData.appliedWindowLevel = preset;
  imageData.voiLutApplied = false;
  imageData.activeWindowLevelIndex = windowLevelIndex;
  UpdateOverlayInfo();
}

void DicomViewerPresenter::OnFitRequested() {
  if (viewerWidget) {
    viewerWidget->FitToWindow();
  }
}

void DicomViewerPresenter::OnToolToggled(ViewerTool tool, bool checked) {
  if (viewerWidget) {
    viewerWidget->SetTool(checked ? tool : ViewerTool::kPan);
  }
}

void DicomViewerPresenter::OnClearMeasurementsRequested() {
  if (viewerWidget) {
    viewerWidget->ClearMeasurements();
  }
}

void DicomViewerPresenter::SetWindowLevelTool() {
  if (viewerWidget) {
    viewerWidget->SetTool(ViewerTool::kWindowLevel);
  }
}

void DicomViewerPresenter::SetPanTool() {
  if (viewerWidget) {
    viewerWidget->SetTool(ViewerTool::kPan);
  }
}

void DicomViewerPresenter::OnCursorPositionChanged(int x, int y, int value) {
  if (view) {
    view->ShowStatusBarMessage(UiStrings::CursorPosition(x, y, value));
  }
}

void DicomViewerPresenter::OnWindowLevelChanged(double center, double width) {
  if (!current) return;

  domain::DicomImageData& imageData = current->image;
  width = std::max(1.0, width);

  domain::WindowLevel windowLevel;
  windowLevel.center = center;
  windowLevel.width = width;
  windowLevel.function = domain::VoiLutFunction::kLinear;

  if (imageData.appliedWindowLevel) {
    windowLevel.function = imageData.appliedWindowLevel->function;
  }
  else if (!imageData.availableWindowLevels.empty()) {
    windowLevel.function = imageData.availableWindowLevels.front().function;
  }

  // Widget já atualizou a imagem via SetWindowLevelIncremental;
  // presenter só sincroniza dados e UI leve, sem reconversão.
  imageData.appliedWindowLevel = windowLevel;
  imageData.voiLutApplied = false;
  imageData.activeWindowLevelIndex = -1;

  if (view) {
    view->UpdateCustomPresetText(FormatCustomText(windowLevel));
    const int presetCount = static_cast<int>(imageData.availableWindowLevels.size());
    const int customIndex = (imageData.hasVoiLut ? 1 : 0) + presetCount;
    view->SelectWindowPreset(customIndex);
  }
  UpdateOverlayInfo();
}

QStringList DicomViewerPresenter::BuildMetadataParts(
  const domain::DicomMetadata& metadata
) {
  QStringList parts;

  if (!metadata.patientName.empty()) {
    parts << UiStrings::PatientInfo(QString::fromStdString(metadata.patientName));
  }

  if (!metadata.studyDescription.empty()) {
    parts << UiStrings::StudyInfo(QString::fromStdString(metadata.studyDescription));
  }

  parts << UiStrings::ModalityInfo(
    QString::fromStdString(metadata.modality.empty() ? "?" : metadata.modality)
  );

  return parts;
}

QString DicomViewerPresenter::FormatImageDimensions(
  const domain::DicomImageData& image
) {
  return UiStrings::ImageDimensions(image.width, image.height);
}

QString DicomViewerPresenter::FormatPixelSpacing(
  const domain::DicomImageData& image
) {
  if (image.HasPixelSpacing()) {
    return UiStrings::PixelSpacing(image.pixelSpacingRow, image.pixelSpacingCol);
  }
  return UiStrings::PixelSpacingUnknown();
}

QString DicomViewerPresenter::FormatWindowLevelStatus(
  const domain::DicomImageData& image
) {
  if (image.voiLutApplied) {
    return UiStrings::VoiLutStatus();
  }

  if (image.appliedWindowLevel) {
    QString text = UiStrings::WindowLevelStatus(
      image.appliedWindowLevel->center, image.appliedWindowLevel->width
    );
    if (image.appliedWindowLevel->function == domain::VoiLutFunction::kSigmoid) {
      text += UiStrings::PresetSigmoidSuffix();
    }
    return text;
  }

  return {};
}

void DicomViewerPresenter::UpdateOverlayInfo() {
  if (!current || !view) return;

  const domain::DicomImageData& image = current->image;
  const domain::DicomMetadata& metadata = current->metadata;

  QStringList parts = BuildMetadataParts(metadata);
  parts << FormatImageDimensions(image);
  parts << FormatPixelSpacing(image);

  QString wlStatus = FormatWindowLevelStatus(image);
  if (!wlStatus.isEmpty()) {
    parts << wlStatus;
  }

  view->SetMetaInfo(parts.join(UiStrings::MetaInfoSeparator()));

  view->SetStatusBarWl(wlStatus);
  view->SetStatusBarDimensions(FormatImageDimensions(image));
}

}  // namespace presentation
