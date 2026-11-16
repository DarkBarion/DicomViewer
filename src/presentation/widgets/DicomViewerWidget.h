#pragma once

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QWidget>
#include <optional>
#include <vector>

#include "domain/DicomImageData.h"
#include "domain/Measurement.h"
#include "presentation/presenter/IDicomViewerWidget.h"
#include "presentation/widgets/HistogramLutBar.h"
#include "presentation/widgets/MeasureOverlayDrawer.h"

namespace presentation {

class DicomViewerWidget : public QWidget,
                          public IDicomViewerWidget {
  Q_OBJECT

 public:
  enum class Tool {
    kPan,
    kMeasure,
    kAngle,
    kCircleRoi,
    kRectRoi,
    kWindowLevel
  };

  explicit DicomViewerWidget(QWidget* newParent = nullptr);

  void DisplayImage(
    const QImage& newImage,
    const domain::DicomImageData& newDicomData
  ) override;

  void ClearImage() override;
  void SetTool(ViewerTool newTool) override;
  void SetToolInternal(Tool newTool);
  void ClearMeasurements() override;
  void SetWindowLevel(const domain::WindowLevel& newWindowLevel) override;
  void RestoreVoiLutPixels() override;
  void FitToWindow() override;

  void RotateClockwise() override;
  void RotateCounterClockwise() override;
  void FlipHorizontal() override;
  void FlipVertical() override;
  void ZoomToOneToOne() override;
  void FitToImageContent() override;

  void SetWindowLevelIncremental(double center, double width, bool isSigmoid = false);

  const domain::DicomImageData& GetDicomData() const override {
    return dicomData;
  }

 signals:
  void CursorPositionChanged(int x, int y, int grayValue);
  void WindowLevelChanged(double center, double width);
  void ZoomChanged(int percent);

 protected:
  void resizeEvent(QResizeEvent* newEvent) override;
  bool eventFilter(QObject* newObject, QEvent* newEvent) override;

 private:
  void FitImageInView();
  double ComputeFitScale() const;
  void UpdateSceneRect();
  void RebuildScene();


  void PositionHistogramOverlay();

  bool HandleMousePress(QMouseEvent* newEvent);
  bool HandleMouseMove(QMouseEvent* newEvent);
  bool HandleMouseRelease(QMouseEvent* newEvent);

  void FinalizeLiveMeasurement();
  void ClearLiveMeasurement();

  void ApplyRotation();
  QRectF ComputeContentBounds() const;

  void DrawAllMeasurements();
  void DrawDistanceMeasurements();
  void DrawAngleMeasurements();
  void DrawCircleRoiMeasurements();
  void DrawRectRoiMeasurements();

  void HandleWheelZoom(QWheelEvent* newWheel);

  void DrawLiveDistance();
  void DrawLiveAngle();
  void DrawLiveRoi();
  void UpdateCursorPosition(QMouseEvent* newEvent);

  void FinalizeDistanceMeasurement(QPointF endPoint);
  void FinalizeAngleMeasurement(QPointF endPoint);
  void FinalizeRoiMeasurements(QPointF endPoint);

  void CancelCurrentMeasurement();
  void DrawLiveDistancePoint();
  void DrawLiveAnglePoint();
  void DrawLiveRoiPoint();

  QGraphicsScene* scene;
  QGraphicsView* view;

  QImage originalImage;
  QImage image;
  domain::DicomImageData dicomData;
  bool hasImage = false;
  bool userHasZoomed = false;

  int rotationDegrees = 0;
  bool flippedH = false;
  bool flippedV = false;

  Tool currentTool = Tool::kPan;

  std::vector<domain::DistanceMeasurement> distanceMeasurements;
  std::vector<domain::AngleMeasurement> angleMeasurements;
  std::vector<domain::CircularRoiMeasurement> circleRoiMeasurements;
  std::vector<domain::RectangularRoiMeasurement> rectRoiMeasurements;

  std::optional<QPointF> measureStart;
  QPointF measureCurrent;
  QPointF angleVertex;
  QGraphicsItem* liveMeasurementItem = nullptr;
  int angleStep = 0;
  int distanceStep = 0;

  domain::WindowLevel windowLevel;

  HistogramLutBar* histogramBar = nullptr;

  bool wlDragging = false;
  QPointF wlDragStartPos;
  double wlDragStartCenter = 0.0;
  double wlDragStartWidth = 1.0;

  QGraphicsPixmapItem* pixmapItem = nullptr;
  MeasureOverlayDrawer drawer;
};

}  // namespace presentation