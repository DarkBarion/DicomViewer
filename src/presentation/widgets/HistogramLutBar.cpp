#include "presentation/widgets/HistogramLutBar.h"

#include <algorithm>
#include <cmath>
#include <QMouseEvent>
#include <QPainter>
#include <QCursor>

namespace presentation {

HistogramLutBar::HistogramLutBar(QWidget* newParent) :
  QWidget(newParent)
{
  setMinimumWidth(180);
  setMaximumWidth(300);
  setMinimumHeight(100);
  setMaximumHeight(120);
  setMouseTracking(true);
  setAttribute(Qt::WA_TranslucentBackground);
}

void HistogramLutBar::SetData(
  const domain::HistogramData& newData,
  const double newCenter,
  const double newWidth
)
{
  histogramData = newData;
  windowCenter = newCenter;
  windowWidth = std::max(1.0, newWidth);
  update();
}

void HistogramLutBar::SetWindowLevel(double newCenter, double newWidth)
{
  windowCenter = newCenter;
  windowWidth = std::max(1.0, newWidth);
  update();
}

void HistogramLutBar::Clear()
{
  histogramData = domain::HistogramData{};
  windowCenter = 0.0;
  windowWidth = 1.0;
  update();
}

void HistogramLutBar::paintEvent(QPaintEvent* /*newEvent*/)
{
  if (histogramData.bins.empty())
    return;

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  DrawBackground(painter);

  int maxCount = 0;
  for (int count : histogramData.bins) {
    maxCount = std::max(maxCount, count);
  }
  if (maxCount <= 0)
    return;

  const int leftX = PixelValueToX(windowCenter - windowWidth / 2.0);
  const int rightX = PixelValueToX(windowCenter + windowWidth / 2.0);

  DrawHistogramBars(painter, maxCount);
  DrawWindowOverlay(painter, leftX, rightX);
  DrawLutBar(painter, leftX, rightX);
  DrawHandles(painter, leftX, rightX);
  DrawLabels(painter, leftX, rightX);
}

void HistogramLutBar::mousePressEvent(QMouseEvent* newEvent)
{
  if (newEvent->button() != Qt::LeftButton || histogramData.bins.empty())
    return;

  const int x = static_cast<int>(newEvent->position().x());
  const Handle handle = HitTest(x);

  if (handle != Handle::kNone) {
    dragging = true;
    dragHandle = handle;
    UpdateWindowLevel(handle, x);
    newEvent->accept();
  }
}

void HistogramLutBar::mouseMoveEvent(QMouseEvent* newEvent)
{
  const int x = static_cast<int>(newEvent->position().x());

  if (dragging) {
    UpdateWindowLevel(dragHandle, x);
    newEvent->accept();
    return;
  }

  const Handle handle = HitTest(x);
  if (handle != Handle::kNone) {
    setCursor(Qt::SplitHCursor);
    mouseInside = true;
  }
  else {
    setCursor(Qt::ArrowCursor);
    mouseInside = false;
  }
  update();
}

void HistogramLutBar::mouseReleaseEvent(QMouseEvent* newEvent)
{
  if (newEvent->button() == Qt::LeftButton) {
    dragging = false;
    dragHandle = Handle::kNone;
    newEvent->accept();
  }
}

int HistogramLutBar::PixelValueToX(double pixelValue) const
{
  const int dw = DrawWidth();
  if (dw <= 0)
    return Layout::padding;

  const double normalized =
    (pixelValue - static_cast<double>(histogramData.pixelMin)) / PixelRange();
  return Layout::padding +
         static_cast<int>(std::clamp(normalized, 0.0, 1.0) * dw);
}

double HistogramLutBar::XToPixelValue(int x) const
{
  const int dw = DrawWidth();
  if (dw <= 0)
    return static_cast<double>(histogramData.pixelMin);

  const double normalized =
    static_cast<double>(
      std::clamp(x, Layout::padding, Layout::padding + dw) - Layout::padding) /
    dw;
  return static_cast<double>(histogramData.pixelMin) +
         normalized * PixelRange();
}

HistogramLutBar::Handle HistogramLutBar::HitTest(int x) const
{
  const int leftX = PixelValueToX(windowCenter - windowWidth / 2.0);
  const int rightX = PixelValueToX(windowCenter + windowWidth / 2.0);

  if (std::abs(x - leftX) <= Layout::handleHitRadius) {
    return Handle::kLeft;
  }
  if (std::abs(x - rightX) <= Layout::handleHitRadius) {
    return Handle::kRight;
  }
  return Handle::kNone;
}

void HistogramLutBar::UpdateWindowLevel(Handle handle, int x)
{
  const double pixelValue = XToPixelValue(x);
  const double clampedValue = std::clamp(
    pixelValue,
    static_cast<double>(histogramData.pixelMin),
    static_cast<double>(histogramData.pixelMax));

  if (handle == Handle::kLeft) {
    const double rightValue = windowCenter + windowWidth / 2.0;
    const double newWidth = std::max(1.0, rightValue - clampedValue);
    windowCenter = clampedValue + newWidth / 2.0;
    windowWidth = newWidth;
  }
  else {
    const double leftValue = windowCenter - windowWidth / 2.0;
    const double newWidth = std::max(1.0, clampedValue - leftValue);
    windowCenter = leftValue + newWidth / 2.0;
    windowWidth = newWidth;
  }

  emit WindowLevelChanged(windowCenter, windowWidth);
  update();
}

void HistogramLutBar::DrawBackground(QPainter& painter) const
{
  painter.fillRect(rect(), Colors::background());
}

void HistogramLutBar::DrawHistogramBars(QPainter& painter, int maxCount) const
{
  const int dw = DrawWidth();
  const int histTop = Layout::padding;
  const int histBottom = histTop + Layout::histogramMaxHeight;

  const int binWidth = dw / kHistogramBins;
  for (int i = 0; i < kHistogramBins; ++i) {
    const int count = histogramData.bins.at(static_cast<std::size_t>(i));
    const int barHeight = static_cast<int>(
      static_cast<double>(count) / maxCount * Layout::histogramMaxHeight);
    const int x = Layout::padding + i * binWidth;

    const double ratio = static_cast<double>(i) / (kHistogramBins - 1);
    const int gray = static_cast<int>(
      Colors::histogramMinGray +
      ratio * (Colors::histogramMaxGray - Colors::histogramMinGray));
    painter.fillRect(
      x,
      histBottom - barHeight,
      std::max(1, binWidth),
      barHeight,
      QColor(gray, gray, gray));
  }
}

void HistogramLutBar::DrawWindowOverlay(
  QPainter& painter,
  int leftX,
  int rightX
) const
{
  const int histTop = Layout::padding;

  painter.fillRect(
    Layout::padding,
    histTop,
    std::max(0, leftX - Layout::padding),
    Layout::histogramMaxHeight,
    Colors::dimOverlay());

  const int rightDimWidth =
    std::max(0, (Layout::padding + DrawWidth()) - rightX);
  painter.fillRect(
    rightX,
    histTop,
    rightDimWidth,
    Layout::histogramMaxHeight,
    Colors::dimOverlay());
}

void HistogramLutBar::DrawLutBar(QPainter& painter, int leftX, int rightX) const
{
  const int lutTop =
    Layout::padding + Layout::histogramMaxHeight + Layout::histogramGap;

  const int gradientWidth = std::max(1, rightX - leftX);
  for (int x = leftX; x <= rightX && x < Layout::padding + DrawWidth(); ++x) {
    const double ratio = static_cast<double>(x - leftX) / gradientWidth;
    const int gray = static_cast<int>(ratio * 255);
    painter
      .fillRect(x, lutTop, 1, Layout::lutBarHeight, QColor(gray, gray, gray));
  }
}

void HistogramLutBar::DrawHandles(QPainter& painter, int leftX, int rightX)
  const
{
  const int histTop = Layout::padding;
  const int histBottom = histTop + Layout::histogramMaxHeight;
  const int histMid = histTop + Layout::histogramMaxHeight / 2;

  painter.setPen(QPen(Colors::handle(), 1));
  painter.setBrush(Colors::handle());

  painter.drawLine(QPointF(leftX, histTop), QPointF(leftX, histBottom));
  {
    QPointF triangle[] = {
      QPointF(leftX, histMid - Layout::arrowHalfHeight),
      QPointF(leftX, histMid + Layout::arrowHalfHeight),
      QPointF(leftX - Layout::arrowHalfHeight, histMid)};
    painter.drawPolygon(triangle, 3);
  }

  painter.drawLine(QPointF(rightX, histTop), QPointF(rightX, histBottom));
  {
    QPointF triangle[] = {
      QPointF(rightX, histMid - Layout::arrowHalfHeight),
      QPointF(rightX, histMid + Layout::arrowHalfHeight),
      QPointF(rightX + Layout::arrowHalfHeight, histMid)};
    painter.drawPolygon(triangle, 3);
  }
}

void HistogramLutBar::DrawLabels(QPainter& painter, int leftX, int rightX) const
{
  const int h = height();
  const int lutBottom = Layout::padding + Layout::histogramMaxHeight +
                        Layout::histogramGap + Layout::lutBarHeight;
  const int labelY = lutBottom + Layout::labelOffsetY;

  if (labelY + Layout::labelHeight > h)
    return;

  const double leftValue = windowCenter - windowWidth / 2.0;
  const double rightValue = windowCenter + windowWidth / 2.0;

  painter.setFont(font());
  painter.setPen(Colors::label());

  painter.drawText(
    leftX - Layout::labelWidth / 2,
    labelY,
    Layout::labelWidth,
    Layout::labelHeight,
    Qt::AlignCenter,
    QString::number(static_cast<int>(leftValue)));
  painter.drawText(
    rightX - Layout::labelWidth / 2,
    labelY,
    Layout::labelWidth,
    Layout::labelHeight,
    Qt::AlignCenter,
    QString::number(static_cast<int>(rightValue)));
}

}  // namespace presentation
