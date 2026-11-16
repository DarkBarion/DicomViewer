#pragma once

#include <QWidget>
#include <vector>

#include "domain/HistogramCalculator.h"

namespace presentation {

class HistogramLutBar : public QWidget {
  Q_OBJECT

 public:
  enum class Handle { kNone, kLeft, kRight };

  explicit HistogramLutBar(QWidget* newParent = nullptr);

  void SetData(
    const domain::HistogramData& newData,
    double newCenter,
    double newWidth
  );

  void SetWindowLevel(double windowCenter, double windowWidth);
  void Clear();

  bool HasData() const { return !histogramData.bins.empty(); }
  double GetWindowCenter() const { return windowCenter; }
  double GetWindowWidth() const { return windowWidth; }

 signals:
  void WindowLevelChanged(double center, double width);

 protected:
  void paintEvent(QPaintEvent* newEvent) override;
  void mousePressEvent(QMouseEvent* newEvent) override;
  void mouseMoveEvent(QMouseEvent* newEvent) override;
  void mouseReleaseEvent(QMouseEvent* newEvent) override;

 private:
  struct Layout {
    static constexpr int padding = 6;
    static constexpr int handleHitRadius = 10;
    static constexpr int histogramMaxHeight = 60;
    static constexpr int lutBarHeight = 16;
    static constexpr int histogramGap = 4;
    static constexpr int arrowHalfHeight = 5;
    static constexpr int labelHeight = 12;
    static constexpr int labelOffsetY = 10;
    static constexpr int labelWidth = 30;
  };

  struct Colors {
    static inline QColor background() { return QColor(10, 10, 10, 180); }
    static inline QColor dimOverlay() { return QColor(0, 0, 0, 140); }
    static inline QColor handle() { return Qt::white; }
    static inline QColor label() { return QColor(200, 200, 200); }
    static constexpr int histogramMinGray = 20;
    static constexpr int histogramMaxGray = 220;
  };

  static constexpr int kHistogramBins = 256;

  int PixelValueToX(double pixelValue) const;
  double XToPixelValue(int x) const;
  Handle HitTest(int x) const;
  void UpdateWindowLevel(Handle handle, int x);

  int DrawWidth() const { return width() - 2 * Layout::padding; }
  double PixelRange() const {
    return std::max(1.0, static_cast<double>(histogramData.pixelMax - histogramData.pixelMin));
  }

  void DrawBackground(QPainter& painter) const;
  void DrawHistogramBars(QPainter& painter, int maxCount) const;
  void DrawWindowOverlay(QPainter& painter, int leftX, int rightX) const;
  void DrawLutBar(QPainter& painter, int leftX, int rightX) const;
  void DrawHandles(QPainter& painter, int leftX, int rightX) const;
  void DrawLabels(QPainter& painter, int leftX, int rightX) const;

  domain::HistogramData histogramData;

  double windowCenter = 0.0;
  double windowWidth = 1.0;

  bool dragging = false;
  Handle dragHandle = Handle::kNone;
  bool mouseInside = false;
};

}  // namespace presentation
