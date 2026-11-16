#pragma once

#include <QImage>

#include "domain/DicomImageData.h"
#include "domain/Measurement.h"

namespace presentation {

enum class ViewerTool {
  kPan,
  kMeasure,
  kAngle,
  kCircleRoi,
  kRectRoi,
  kWindowLevel  // Ajuste de W/L por arraste na imagem
};

class IDicomViewerWidget {
 public:
  virtual ~IDicomViewerWidget() = default;

  virtual void DisplayImage(
    const QImage& newImage,
    const domain::DicomImageData& newDicomData
  ) = 0;

  virtual void ClearImage() = 0;

  virtual void SetTool(ViewerTool newTool) = 0;

  virtual void SetWindowLevel(const domain::WindowLevel& newWindowLevel) = 0;

  virtual void RestoreVoiLutPixels() = 0;

  virtual void FitToWindow() = 0;

  virtual void ClearMeasurements() = 0;

  virtual void RotateClockwise() = 0;
  virtual void RotateCounterClockwise() = 0;
  virtual void FlipHorizontal() = 0;
  virtual void FlipVertical() = 0;
  virtual void ZoomToOneToOne() = 0;
  virtual void FitToImageContent() = 0;

  virtual const domain::DicomImageData& GetDicomData() const = 0;
};

}  // namespace presentation
