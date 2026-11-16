#include "presentation/widgets/DicomViewerWidget.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QMouseEvent>
#include <QTransform>
#include <QVBoxLayout>
#include <QWheelEvent>

#include "domain/HistogramCalculator.h"
#include "presentation/widgets/ImageConverter.h"
#include "presentation/widgets/HistogramLutBar.h"
#include "presentation/widgets/MeasureOverlayDrawer.h"

namespace presentation {
namespace {

constexpr double kMinZoomFactor = 0.1;
constexpr double kMaxZoomFactor = 20.0;
constexpr double kWheelZoomStep = 1.15;
constexpr int kBorderGray = 128;
constexpr int kHistogramWidth = 220;
constexpr int kHistogramHeight = 100;
constexpr int kHistogramMargin = 8;
constexpr double kWlDragSensitivity = 0.5;
constexpr double kMeasureMinDistance = 3.0;

}  // namespace

DicomViewerWidget::DicomViewerWidget(QWidget* newParent) :
  QWidget(newParent),
  scene(new QGraphicsScene(this)),
  view(new QGraphicsView(scene, this)),
  histogramBar(new HistogramLutBar(this)),
  drawer(scene)
{
  view->setRenderHint(QPainter::SmoothPixmapTransform, true);
  view->setDragMode(QGraphicsView::ScrollHandDrag);
  view->setBackgroundBrush(Qt::black);
  view->setMouseTracking(true);
  view->viewport()->installEventFilter(this);

  view->setAlignment(Qt::AlignCenter);
  view->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
  view->setResizeAnchor(QGraphicsView::AnchorViewCenter);

  view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(view);

  histogramBar->setFixedSize(kHistogramWidth, kHistogramHeight);
  histogramBar->hide();

  connect(histogramBar, &HistogramLutBar::WindowLevelChanged,
          this, &DicomViewerWidget::WindowLevelChanged);
}

void DicomViewerWidget::DisplayImage(
  const QImage& newImage,
  const domain::DicomImageData& newDicomData
)
{
  originalImage = newImage;
  image = newImage;
  dicomData = newDicomData;
  hasImage = true;
  rotationDegrees = 0;
  flippedH = false;
  flippedV = false;

  if (dicomData.appliedWindowLevel) {
    windowLevel = dicomData.appliedWindowLevel.value();
  }

  RebuildScene();
  FitImageInView();

  if (!dicomData.rawPixels.empty()) {
    const domain::HistogramData histData =
      domain::HistogramCalculator::Compute(dicomData.rawPixels);
    histogramBar->SetData(histData, windowLevel.center, windowLevel.width);
    histogramBar->show();
    PositionHistogramOverlay();
  }
}

void DicomViewerWidget::PositionHistogramOverlay() {
  if (!view || !hasImage) return;
  histogramBar->move(
    kHistogramMargin,
    view->viewport()->height() - kHistogramHeight - kHistogramMargin
  );
}

void DicomViewerWidget::ClearImage() {
  image = {};
  hasImage = false;
  userHasZoomed = false;
  rotationDegrees = 0;
  flippedH = false;
  flippedV = false;
  distanceMeasurements.clear();
  angleMeasurements.clear();
  circleRoiMeasurements.clear();
  rectRoiMeasurements.clear();
  scene->clear();
  liveMeasurementItem = nullptr;
  histogramBar->Clear();
  histogramBar->hide();
}

void DicomViewerWidget::SetTool(ViewerTool newTool) {
  Tool internalTool;
  switch (newTool) {
    case ViewerTool::kMeasure:     internalTool = Tool::kMeasure; break;
    case ViewerTool::kAngle:       internalTool = Tool::kAngle; break;
    case ViewerTool::kCircleRoi:   internalTool = Tool::kCircleRoi; break;
    case ViewerTool::kRectRoi:     internalTool = Tool::kRectRoi; break;
    case ViewerTool::kWindowLevel: internalTool = Tool::kWindowLevel; break;
    default:                       internalTool = Tool::kPan; break;
  }
  SetToolInternal(internalTool);
}

void DicomViewerWidget::SetToolInternal(Tool newTool) {
  currentTool = newTool;
  measureStart.reset();
  angleStep = 0;
  distanceStep = 0;
  angleVertex = {};
  ClearLiveMeasurement();
}

void DicomViewerWidget::ClearMeasurements() {
  distanceMeasurements.clear();
  angleMeasurements.clear();
  circleRoiMeasurements.clear();
  rectRoiMeasurements.clear();
  RebuildScene();
}

void DicomViewerWidget::RotateClockwise() {
  if (!hasImage) return;
  rotationDegrees = (rotationDegrees + 90) % 360;
  ApplyRotation();
}

void DicomViewerWidget::RotateCounterClockwise() {
  if (!hasImage) return;
  rotationDegrees = (rotationDegrees + 270) % 360;
  ApplyRotation();
}

void DicomViewerWidget::FlipHorizontal() {
  if (!hasImage) return;
  flippedH = !flippedH;
  ApplyRotation();
}

void DicomViewerWidget::FlipVertical() {
  if (!hasImage) return;
  flippedV = !flippedV;
  ApplyRotation();
}

void DicomViewerWidget::ApplyRotation() {
  if (originalImage.isNull()) return;

  QImage result = originalImage;

  if (rotationDegrees != 0) {
    QTransform transform;
    transform.rotate(rotationDegrees);
    result = result.transformed(transform, Qt::SmoothTransformation);
  }

  if (flippedH || flippedV) {
    Qt::Orientations orientations;
    if (flippedH) orientations |= Qt::Horizontal;
    if (flippedV) orientations |= Qt::Vertical;
    result = result.flipped(orientations);
  }

  image = result;
  dicomData.width = image.width();
  dicomData.height = image.height();

  RebuildScene();
  FitImageInView();
}

void DicomViewerWidget::ZoomToOneToOne() {
  if (!hasImage) return;

  view->resetTransform();
  const QRectF imageBounds(0, 0, dicomData.width, dicomData.height);
  view->setSceneRect(imageBounds);

  QRectF target = ComputeContentBounds();
  if (target.isEmpty() || target.width() < 2 || target.height() < 2) {
    target = imageBounds;
  }

  view->centerOn(target.center());
  view->ensureVisible(target);
  userHasZoomed = true;

  const double fitScale = ComputeFitScale();
  const double currentScale = view->transform().m11();
  const int percent = static_cast<int>(currentScale / fitScale * 100.0);
  emit ZoomChanged(percent);
}

void DicomViewerWidget::FitToImageContent() {
  if (!hasImage || image.isNull()) return;

  const QRectF content = ComputeContentBounds();
  if (content.isEmpty() || content.width() < 2 || content.height() < 2) {
    FitImageInView();
    return;
  }

  view->fitInView(content, Qt::KeepAspectRatio);
  UpdateSceneRect();
  view->centerOn(content.center());
  userHasZoomed = true;
}

QRectF DicomViewerWidget::ComputeContentBounds() const {
  if (image.isNull()) return {};

  const int w = image.width();
  const int h = image.height();

  // Determinar threshold dinâmico amostrando as bordas da imagem.
  // O fundo de imagens DICOM (especialmente mamografia) é preto nas bordas.
  // Se as bordas não forem escuras, a imagem não tem fundo recortável.
  constexpr int sampleDepth = 16;
  std::vector<std::uint8_t> edgeSamples;
  edgeSamples.reserve(static_cast<std::size_t>(w) * 2 + static_cast<std::size_t>(h) * 2);

  const int depth = std::min(sampleDepth, std::min(w, h));
  for (int i = 0; i < depth; ++i) {
    const std::uint8_t* top = image.constScanLine(i);
    if (top) {
      edgeSamples.push_back(top[0]);
      edgeSamples.push_back(top[w - 1]);
    }
    const std::uint8_t* bottom = image.constScanLine(h - 1 - i);
    if (bottom) {
      edgeSamples.push_back(bottom[0]);
      edgeSamples.push_back(bottom[w - 1]);
    }
    const std::uint8_t* left = image.constScanLine(i);
    if (left) edgeSamples.push_back(left[0]);
    const std::uint8_t* right = image.constScanLine(i);
    if (right) edgeSamples.push_back(right[w - 1]);
  }

  if (edgeSamples.empty()) return {};

  // Calcular média e desvio padrão das bordas para determinar threshold
  double sum = 0.0;
  for (std::uint8_t v : edgeSamples) sum += v;
  const double sampleCount = static_cast<double>(edgeSamples.size());
  const double mean = sum / sampleCount;

  // Se a média das bordas for alta, a imagem não tem fundo preto detectável
  if (mean > 30.0) return {};

  double variance = 0.0;
  for (std::uint8_t v : edgeSamples) {
    const double diff = v - mean;
    variance += diff * diff;
  }
  variance /= sampleCount;
  const double stddev = std::sqrt(variance);

  // Threshold: média + 3 desvios padrão, garantindo mínimo de 5
  const int threshold = std::max(5, static_cast<int>(mean + 3.0 * stddev));

  int minX = w, minY = h, maxX = 0, maxY = 0;
  for (int y = 0; y < h; ++y) {
    const std::uint8_t* scanLine = image.constScanLine(y);
    if (!scanLine) continue;
    for (int x = 0; x < w; ++x) {
      if (scanLine[x] > threshold) {
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
      }
    }
  }

  if (minX > maxX || minY > maxY) return {};
  return QRectF(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

void DicomViewerWidget::SetWindowLevel(
  const domain::WindowLevel& newWindowLevel
) {
  if (dicomData.rawPixels.empty() || !newWindowLevel.IsValid()) return;
  windowLevel = newWindowLevel;
  image = ImageConverter::ToQImageWithWindow(dicomData, newWindowLevel);
  RebuildScene();
  const domain::HistogramData histData =
    domain::HistogramCalculator::Compute(dicomData.rawPixels);
  histogramBar->SetData(histData, newWindowLevel.center, newWindowLevel.width);
  histogramBar->show();
  PositionHistogramOverlay();
}

void DicomViewerWidget::RestoreVoiLutPixels() {
  if (dicomData.voiLutPixels.empty()) return;
  image = QImage(
    dicomData.voiLutPixels.data(),
    dicomData.width,
    dicomData.height,
    dicomData.width,
    QImage::Format_Grayscale8
  ).copy();
  dicomData.pixels = dicomData.voiLutPixels;
  RebuildScene();
}

void DicomViewerWidget::FitToWindow() {
  FitImageInView();
}

void DicomViewerWidget::resizeEvent(QResizeEvent* newEvent) {
  QWidget::resizeEvent(newEvent);
  if (!userHasZoomed) {
    FitImageInView();
  } else {
    UpdateSceneRect();
  }
  if (hasImage && histogramBar->HasData()) {
    PositionHistogramOverlay();
  }
}

void DicomViewerWidget::FitImageInView() {
  if (scene->items().isEmpty()) return;
  const QRectF imageBounds(0, 0, dicomData.width, dicomData.height);
  view->fitInView(imageBounds, Qt::KeepAspectRatio);
  UpdateSceneRect();
  view->centerOn(imageBounds.center());
  userHasZoomed = false;
  emit ZoomChanged(100);
}

double DicomViewerWidget::ComputeFitScale() const {
  const QRectF imageBounds(0, 0, dicomData.width, dicomData.height);
  if (imageBounds.isEmpty()) return 1.0;
  const double scaleX =
    static_cast<double>(view->viewport()->width()) / imageBounds.width();
  const double scaleY =
    static_cast<double>(view->viewport()->height()) / imageBounds.height();
  return std::min(scaleX, scaleY);
}

void DicomViewerWidget::UpdateSceneRect() {
  if (scene->items().isEmpty() || dicomData.width <= 0) return;
  const QRectF imageBounds(0, 0, dicomData.width, dicomData.height);

  const double scale = view->transform().m11();
  if (scale <= 0.0) return;

  const double halfViewportWidth =
    static_cast<double>(view->viewport()->width()) / scale / 2.0;
  const double halfViewportHeight =
    static_cast<double>(view->viewport()->height()) / scale / 2.0;

  scene->setSceneRect(
    imageBounds.adjusted(
      -halfViewportWidth,
      -halfViewportHeight,
      halfViewportWidth,
      halfViewportHeight
    )
  );
}

void DicomViewerWidget::RebuildScene() {
  scene->clear();
  liveMeasurementItem = nullptr;
  pixmapItem = nullptr;
  if (!image.isNull()) {
    pixmapItem = scene->addPixmap(QPixmap::fromImage(image));
    const QRectF imageBounds(0, 0, dicomData.width, dicomData.height);
    QGraphicsRectItem* border = scene->addRect(
      imageBounds,
      QPen(QColor(kBorderGray, kBorderGray, kBorderGray), 0));
    border->setZValue(5);
  }
  DrawAllMeasurements();
}

void DicomViewerWidget::DrawAllMeasurements() {
  DrawDistanceMeasurements();
  DrawAngleMeasurements();
  DrawCircleRoiMeasurements();
  DrawRectRoiMeasurements();
}

void DicomViewerWidget::DrawDistanceMeasurements() {
  for (const domain::DistanceMeasurement& m : distanceMeasurements) {
    drawer.DrawDistance(m);
  }
}

void DicomViewerWidget::DrawAngleMeasurements() {
  for (const domain::AngleMeasurement& m : angleMeasurements) {
    drawer.DrawAngle(m);
  }
}

void DicomViewerWidget::DrawCircleRoiMeasurements() {
  for (const domain::CircularRoiMeasurement& m : circleRoiMeasurements) {
    drawer.DrawCircleRoi(m);
  }
}

void DicomViewerWidget::DrawRectRoiMeasurements() {
  for (const domain::RectangularRoiMeasurement& m : rectRoiMeasurements) {
    drawer.DrawRectRoi(m);
  }
}


void DicomViewerWidget::HandleWheelZoom(QWheelEvent* newWheel) {
  const double factor =
    newWheel->angleDelta().y() > 0 ? kWheelZoomStep : 1.0 / kWheelZoomStep;

  const double currentScale = view->transform().m11();
  const double newScale = currentScale * factor;
  const double fitScale = ComputeFitScale();
  const double minScale = fitScale * kMinZoomFactor;
  const double maxScale = fitScale * kMaxZoomFactor;

  if (newScale < minScale || newScale > maxScale) {
    return;
  }

  view->scale(factor, factor);
  UpdateSceneRect();
  userHasZoomed = true;

  const double zoomedScale = view->transform().m11();
  const int percent = static_cast<int>(zoomedScale / fitScale * 100.0);
  emit ZoomChanged(percent);
}

void DicomViewerWidget::SetWindowLevelIncremental(
  double center,
  double width,
  bool isSigmoid
) {
  if (dicomData.rawPixels.empty() || dicomData.width <= 0 || dicomData.height <= 0) {
    return;
  }

  const std::vector<std::uint8_t> lut = ImageConverter::BuildWindowLut(center, width, isSigmoid);

  QImage img(dicomData.width, dicomData.height, QImage::Format_Grayscale8);

  for (int y = 0; y < dicomData.height; ++y) {
    std::uint8_t* line = img.scanLine(y);
    const std::size_t rowOffset = static_cast<std::size_t>(y) * static_cast<std::size_t>(dicomData.width);
    for (int x = 0; x < dicomData.width; ++x) {
      const std::size_t index = rowOffset + static_cast<std::size_t>(x);
      line[x] = lut[dicomData.rawPixels[index]];
    }
  }

  if (pixmapItem != nullptr) {
    pixmapItem->setPixmap(QPixmap::fromImage(img));
  }

  if (histogramBar != nullptr) {
    histogramBar->SetWindowLevel(center, width);
  }
}

bool DicomViewerWidget::eventFilter(
  QObject* newObject,
  QEvent* newEvent
) {
  if (newObject != view->viewport()) {
    return QWidget::eventFilter(newObject, newEvent);
  }

  switch (newEvent->type()) {
    case QEvent::Wheel:
      if (hasImage) {
        QWheelEvent* wheel = static_cast<QWheelEvent*>(newEvent);
        HandleWheelZoom(wheel);
        return true;
      }
      break;

    case QEvent::MouseButtonPress: {
      auto* mouseEvent = dynamic_cast<QMouseEvent*>(newEvent);
      if (HandleMousePress(mouseEvent)) return true;
      break;
    }
    case QEvent::MouseMove: {
      auto* mouseEvent = dynamic_cast<QMouseEvent*>(newEvent);
      if (HandleMouseMove(mouseEvent)) return true;
      break;
    }
    case QEvent::MouseButtonRelease: {
      auto* mouseEvent = dynamic_cast<QMouseEvent*>(newEvent);
      if (HandleMouseRelease(mouseEvent)) return true;
      break;
    }
    default:
      break;
  }
  return QWidget::eventFilter(newObject, newEvent);
}

bool DicomViewerWidget::HandleMousePress(QMouseEvent* newEvent) {
  if (!hasImage) return false;

  if (newEvent->button() == Qt::RightButton) {
    CancelCurrentMeasurement();
    return true;
  }

  if (newEvent->button() == Qt::LeftButton
      && currentTool == Tool::kMeasure) {
    const QPointF scenePos = view->mapToScene(newEvent->pos());
    if (distanceStep == 0) {
      measureStart = scenePos;
      distanceStep = 1;
      DrawLiveDistancePoint();
    } else if (distanceStep == 1) {
      FinalizeDistanceMeasurement(scenePos);
      distanceStep = 0;
    }
    return true;
  }

  if (newEvent->button() == Qt::LeftButton
      && currentTool == Tool::kAngle) {
    const QPointF scenePos = view->mapToScene(newEvent->pos());
    if (angleStep == 0) {
      measureStart = scenePos;
      angleStep = 1;
      DrawLiveAnglePoint();
    } else if (angleStep == 1) {
      angleVertex = scenePos;
      angleStep = 2;
      DrawLiveAnglePoint();
    } else if (angleStep == 2) {
      FinalizeAngleMeasurement(scenePos);
      angleStep = 0;
    }
    return true;
  }

  if (newEvent->button() == Qt::LeftButton
      && (currentTool == Tool::kCircleRoi || currentTool == Tool::kRectRoi)) {
    const QPointF scenePos = view->mapToScene(newEvent->pos());
    if (measureStart.has_value() == false) {
      measureStart = scenePos;
      measureCurrent = scenePos;
      DrawLiveRoiPoint();
    } else {
      FinalizeRoiMeasurements(scenePos);
    }
    return true;
  }

  if ((newEvent->button() == Qt::LeftButton && currentTool == Tool::kWindowLevel)
      || newEvent->button() == Qt::MiddleButton) {
    wlDragging = true;
    wlDragStartPos = newEvent->pos();
    wlDragStartCenter = windowLevel.center;
    wlDragStartWidth = windowLevel.width;
    return true;
  }

  return false;
}

bool DicomViewerWidget::HandleMouseMove(QMouseEvent* newEvent) {
  if (wlDragging) {
    const QPoint delta = newEvent->pos() - wlDragStartPos.toPoint();
    const double widthDelta = static_cast<double>(delta.x()) * kWlDragSensitivity;
    const double centerDelta = -static_cast<double>(delta.y()) * kWlDragSensitivity;
    const double newCenter = std::max(0.0, wlDragStartCenter + centerDelta);
    const double newWidth = std::max(1.0, wlDragStartWidth + widthDelta);

    const bool isSigmoid = windowLevel.function == domain::VoiLutFunction::kSigmoid;
    SetWindowLevelIncremental(newCenter, newWidth, isSigmoid);

    windowLevel.center = newCenter;
    windowLevel.width = newWidth;

    emit WindowLevelChanged(newCenter, newWidth);
    return true;
  }

  if (currentTool == Tool::kMeasure && distanceStep == 1) {
    measureCurrent = view->mapToScene(newEvent->pos());
    DrawLiveDistance();
    return true;
  }

  if (currentTool == Tool::kAngle && angleStep >= 1) {
    measureCurrent = view->mapToScene(newEvent->pos());
    DrawLiveAngle();
    return true;
  }

  if (measureStart.has_value()
      && (currentTool == Tool::kCircleRoi || currentTool == Tool::kRectRoi)) {
    measureCurrent = view->mapToScene(newEvent->pos());
    DrawLiveRoi();
    return true;
  }

  UpdateCursorPosition(newEvent);
  return false;
}

void DicomViewerWidget::DrawLiveDistance() {
  ClearLiveMeasurement();
  liveMeasurementItem = scene->addLine(
    QLineF(*measureStart, measureCurrent),
    QPen(drawer.GetStyles().distanceColor, 1, Qt::DashLine)
  );
  liveMeasurementItem->setZValue(drawer.GetStyles().lineZ);
}

void DicomViewerWidget::DrawLiveAngle() {
  ClearLiveMeasurement();
  if (angleStep == 1) {
    QGraphicsLineItem* line = scene->addLine(
      QLineF(*measureStart, measureCurrent),
      QPen(drawer.GetStyles().angleColor, 1, Qt::DashLine)
    );
    line->setZValue(drawer.GetStyles().lineZ);
    liveMeasurementItem = line;
  } else if (angleStep == 2) {
    QGraphicsLineItem* line1 = scene->addLine(
      QLineF(*measureStart, angleVertex),
      QPen(drawer.GetStyles().angleColor, 1, Qt::DashLine)
    );
    QGraphicsLineItem* line2 = scene->addLine(
      QLineF(angleVertex, measureCurrent),
      QPen(drawer.GetStyles().angleColor, 1, Qt::DashLine)
    );
    line1->setZValue(drawer.GetStyles().lineZ);
    line2->setZValue(drawer.GetStyles().lineZ);
    liveMeasurementItem = line2;
  }
}

void DicomViewerWidget::DrawLiveRoi() {
  ClearLiveMeasurement();

  if (currentTool == Tool::kCircleRoi) {
    const double dx = measureCurrent.x() - measureStart->x();
    const double dy = measureCurrent.y() - measureStart->y();
    const double radius = std::sqrt(dx * dx + dy * dy);
    const double diameter = 2.0 * radius;
    liveMeasurementItem = scene->addEllipse(
      measureStart->x() - radius, measureStart->y() - radius,
      diameter, diameter,
      QPen(drawer.GetStyles().circleRoiColor, 1, Qt::DashLine), QBrush()
    );
  } else {
    const QRectF rect(*measureStart, measureCurrent);
    liveMeasurementItem = scene->addRect(
      rect, QPen(drawer.GetStyles().rectRoiColor, 1, Qt::DashLine), QBrush()
    );
  }
  static_cast<QGraphicsItem*>(liveMeasurementItem)->setZValue(drawer.GetStyles().lineZ);
}

void DicomViewerWidget::UpdateCursorPosition(QMouseEvent* newEvent) {
  if (!hasImage) return;
  const QPointF scenePosition = view->mapToScene(newEvent->pos());
  const int px = static_cast<int>(scenePosition.x());
  const int py = static_cast<int>(scenePosition.y());
  if (px >= 0 && py >= 0
      && px < dicomData.width && py < dicomData.height
      && !dicomData.rawPixels.empty()) {
    const std::size_t index = static_cast<std::size_t>(py)
                     * static_cast<std::size_t>(dicomData.width)
                     + static_cast<std::size_t>(px);
    emit CursorPositionChanged(px, py, dicomData.rawPixels.at(index));
  }
}

bool DicomViewerWidget::HandleMouseRelease(QMouseEvent* newEvent) {
  if ((newEvent->button() == Qt::LeftButton || newEvent->button() == Qt::MiddleButton)
      && wlDragging) {
    wlDragging = false;
    if (!dicomData.rawPixels.empty() && windowLevel.IsValid()) {
      image = ImageConverter::ToQImageWithWindow(dicomData, windowLevel);
      if (pixmapItem) {
        pixmapItem->setPixmap(QPixmap::fromImage(image));
      }
    }
    return true;
  }

  return false;
}

void DicomViewerWidget::FinalizeDistanceMeasurement(QPointF endPoint) {
  const double dx = endPoint.x() - measureStart->x();
  const double dy = endPoint.y() - measureStart->y();
  if (std::hypot(dx, dy) > kMeasureMinDistance) {
    domain::DistanceMeasurement measurement(
      *measureStart,
      endPoint,
      dicomData.pixelSpacingRow,
      dicomData.pixelSpacingCol
    );
    distanceMeasurements.push_back(measurement);
    drawer.DrawDistance(measurement);
  }
  FinalizeLiveMeasurement();
}

void DicomViewerWidget::FinalizeAngleMeasurement(QPointF endPoint) {
  const double dx1 = angleVertex.x() - measureStart->x();
  const double dy1 = angleVertex.y() - measureStart->y();
  if (std::hypot(dx1, dy1) > kMeasureMinDistance) {
    domain::AngleMeasurement measurement(
      angleVertex,
      *measureStart,
      endPoint,
      dicomData.pixelSpacingRow,
      dicomData.pixelSpacingCol
    );
    angleMeasurements.push_back(measurement);
    drawer.DrawAngle(measurement);
  }
  FinalizeLiveMeasurement();
}

void DicomViewerWidget::FinalizeRoiMeasurements(QPointF endPoint) {
  if (currentTool == Tool::kCircleRoi) {
    const double dx = endPoint.x() - measureStart->x();
    const double dy = endPoint.y() - measureStart->y();
    const double radius = std::sqrt(dx * dx + dy * dy);
    if (radius > kMeasureMinDistance) {
      domain::PixelStatistics stats = domain::ComputeCircularRoiStatistics(
        dicomData.rawPixels,
        dicomData.width, dicomData.height,
        *measureStart, radius
      );
      domain::CircularRoiMeasurement measurement(
        *measureStart, radius,
        dicomData.pixelSpacingRow, dicomData.pixelSpacingCol,
        stats
      );
      circleRoiMeasurements.push_back(measurement);
      drawer.DrawCircleRoi(measurement);
    }
  } else {
    const QRectF rect(*measureStart, endPoint);
    if (rect.width() > kMeasureMinDistance && rect.height() > kMeasureMinDistance) {
      domain::PixelStatistics stats = domain::ComputeRectangularRoiStatistics(
        dicomData.rawPixels,
        dicomData.width, dicomData.height,
        rect
      );
      domain::RectangularRoiMeasurement measurement(
        *measureStart, endPoint,
        dicomData.pixelSpacingRow, dicomData.pixelSpacingCol,
        stats
      );
      rectRoiMeasurements.push_back(measurement);
      drawer.DrawRectRoi(measurement);
    }
  }
  FinalizeLiveMeasurement();
}

void DicomViewerWidget::FinalizeLiveMeasurement() {
  measureStart.reset();
  angleStep = 0;
  distanceStep = 0;
  angleVertex = {};
  ClearLiveMeasurement();
}

void DicomViewerWidget::ClearLiveMeasurement() {
  if (liveMeasurementItem) {
    scene->removeItem(liveMeasurementItem);
    delete liveMeasurementItem;
    liveMeasurementItem = nullptr;
  }
}

void DicomViewerWidget::CancelCurrentMeasurement() {
  if (currentTool == Tool::kMeasure && distanceStep > 0) {
    distanceStep = 0;
    FinalizeLiveMeasurement();
    return;
  }
  if (currentTool == Tool::kAngle && angleStep > 0) {
    angleStep = 0;
    FinalizeLiveMeasurement();
    return;
  }
  if (currentTool == Tool::kCircleRoi || currentTool == Tool::kRectRoi) {
    if (measureStart.has_value()) {
      FinalizeLiveMeasurement();
    }
    return;
  }
}

void DicomViewerWidget::DrawLiveDistancePoint() {
  ClearLiveMeasurement();
  liveMeasurementItem = scene->addEllipse(
    -drawer.GetStyles().endPointRadius, -drawer.GetStyles().endPointRadius,
    2 * drawer.GetStyles().endPointRadius, 2 * drawer.GetStyles().endPointRadius,
    QPen(drawer.GetStyles().distanceColor, 2), QBrush()
  );
  static_cast<QGraphicsEllipseItem*>(liveMeasurementItem)->setPos(*measureStart);
  liveMeasurementItem->setZValue(drawer.GetStyles().lineZ);
}

void DicomViewerWidget::DrawLiveAnglePoint() {
  ClearLiveMeasurement();
  const double r = drawer.GetStyles().endPointRadius;
  const QColor& color = drawer.GetStyles().angleColor;
  QGraphicsEllipseItem* first = scene->addEllipse(
    -r, -r, 2 * r, 2 * r, QPen(color, 2), QBrush()
  );
  first->setPos(*measureStart);
  first->setZValue(drawer.GetStyles().lineZ);
  if (angleStep >= 2) {
    QGraphicsEllipseItem* second = scene->addEllipse(
      -r, -r, 2 * r, 2 * r, QPen(color, 2), QBrush()
    );
    second->setPos(angleVertex);
    second->setZValue(drawer.GetStyles().lineZ);
    liveMeasurementItem = second;
  } else {
    liveMeasurementItem = first;
  }
}

void DicomViewerWidget::DrawLiveRoiPoint() {
  ClearLiveMeasurement();
  liveMeasurementItem = scene->addEllipse(
    -drawer.GetStyles().endPointRadius, -drawer.GetStyles().endPointRadius,
    2 * drawer.GetStyles().endPointRadius, 2 * drawer.GetStyles().endPointRadius,
    QPen(drawer.GetStyles().circleRoiColor, 2), QBrush()
  );
  static_cast<QGraphicsEllipseItem*>(liveMeasurementItem)->setPos(*measureStart);
  liveMeasurementItem->setZValue(drawer.GetStyles().lineZ);
}

}  // namespace presentation
