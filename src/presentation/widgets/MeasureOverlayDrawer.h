#pragma once

#include <QColor>
#include <QPointF>

#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QPainterPath>

#include "domain/Measurement.h"

namespace presentation {

struct MeasureOverlayStyles {
  QColor distanceColor = QColor(255, 220, 0);
  QColor angleColor = QColor(0, 255, 150);
  QColor circleRoiColor = QColor(100, 150, 255);
  QColor rectRoiColor = QColor(255, 100, 200);

  double endPointRadius = 3.0;
  double lineWidth = 1.0;
  double textOffset = 14.0;
  double arcRadius = 30.0;

  int lineZ = 10;
  int pointZ = 11;
  int textZ = 12;
};

class MeasureOverlayDrawer {
 public:
  explicit MeasureOverlayDrawer(
    QGraphicsScene* newScene,
    MeasureOverlayStyles newStyles = {}
  ) :
    styles(newStyles), scene(newScene) {}

  const MeasureOverlayStyles& GetStyles() const { return styles; }

  void DrawDistance(const domain::DistanceMeasurement&);

  void DrawAngle(const domain::AngleMeasurement&);

  void DrawCircleRoi(const domain::CircularRoiMeasurement&);

  void DrawRectRoi(const domain::RectangularRoiMeasurement&);

 private:
  MeasureOverlayStyles styles;
  QGraphicsScene* scene;

  QGraphicsEllipseItem* AddEndPoint(QPointF, const QColor&);
  QGraphicsSimpleTextItem* AddText(const QString&, QPointF, const QColor&);
};

namespace MeasureFormat {
QString Distance(const domain::DistanceMeasurement&);
QString Angle(const domain::AngleMeasurement&);
QString CircleRoi(const domain::CircularRoiMeasurement&);
QString RectRoi(const domain::RectangularRoiMeasurement&);
}  // namespace MeasureFormat

}  // namespace presentation
