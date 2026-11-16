#include "presentation/widgets/MeasureOverlayDrawer.h"

#include <cmath>

#include "presentation/resources/UiStrings.h"

namespace presentation {

QGraphicsEllipseItem* MeasureOverlayDrawer::AddEndPoint(QPointF pos, const QColor& color) {
  QGraphicsEllipseItem* circle = scene->addEllipse(
    -styles.endPointRadius, -styles.endPointRadius,
    2 * styles.endPointRadius, 2 * styles.endPointRadius,
    QPen(color, styles.lineWidth), QBrush()
  );
  circle->setPos(pos);
  circle->setZValue(styles.pointZ);
  return circle;
}

QGraphicsSimpleTextItem* MeasureOverlayDrawer::AddText(
  const QString& text, QPointF pos, const QColor& color
) {
  QGraphicsSimpleTextItem* item = scene->addSimpleText(text);
  item->setBrush(color);
  item->setPos(pos);
  item->setZValue(styles.textZ);
  return item;
}

void MeasureOverlayDrawer::DrawDistance(const domain::DistanceMeasurement& m) {
  QGraphicsLineItem* line = scene->addLine(
    QLineF(m.startPoint, m.endPoint),
    QPen(styles.distanceColor, styles.lineWidth)
  );
  line->setZValue(styles.lineZ);

  AddEndPoint(m.startPoint, styles.distanceColor);
  AddEndPoint(m.endPoint, styles.distanceColor);

  const QPointF mid = m.MidPoint();
  const double angle = m.Angle();
  const double perpX = -std::sin(angle) * styles.textOffset;
  const double perpY = std::cos(angle) * styles.textOffset;
  AddText(MeasureFormat::Distance(m), mid + QPointF(perpX, perpY), styles.distanceColor);
}

void MeasureOverlayDrawer::DrawAngle(const domain::AngleMeasurement& m) {
  QGraphicsLineItem* line1 = scene->addLine(
    QLineF(m.point1, m.vertex),
    QPen(styles.angleColor, styles.lineWidth)
  );
  line1->setZValue(styles.lineZ);

  QGraphicsLineItem* line2 = scene->addLine(
    QLineF(m.vertex, m.point2),
    QPen(styles.angleColor, styles.lineWidth)
  );
  line2->setZValue(styles.lineZ);

  const double arcR = styles.arcRadius;
  const double angle1 = std::atan2(m.point1.y() - m.vertex.y(), m.point1.x() - m.vertex.x());
  const double angle2 = std::atan2(m.point2.y() - m.vertex.y(), m.point2.x() - m.vertex.x());

  QPainterPath arcPath;
  arcPath.moveTo(
    m.vertex.x() + arcR * std::cos(angle1),
    m.vertex.y() + arcR * std::sin(angle1)
  );
  arcPath.arcTo(
    QRectF(m.vertex.x() - arcR, m.vertex.y() - arcR, 2 * arcR, 2 * arcR),
    angle1 * 180.0 / M_PI,
    (angle2 - angle1) * 180.0 / M_PI
  );
  QGraphicsPathItem* arc = scene->addPath(
    arcPath, QPen(styles.angleColor, styles.lineWidth), QBrush()
  );
  arc->setZValue(styles.lineZ);

  AddEndPoint(m.vertex, styles.angleColor);
  AddEndPoint(m.point1, styles.angleColor);
  AddEndPoint(m.point2, styles.angleColor);

  AddText(MeasureFormat::Angle(m), m.vertex + QPointF(10, -arcR - 16), styles.angleColor);
}

void MeasureOverlayDrawer::DrawCircleRoi(const domain::CircularRoiMeasurement& m) {
  const double diameter = 2.0 * m.radius;
  QGraphicsEllipseItem* ellipse = scene->addEllipse(
    m.center.x() - m.radius, m.center.y() - m.radius,
    diameter, diameter,
    QPen(styles.circleRoiColor, styles.lineWidth), QBrush()
  );
  ellipse->setZValue(styles.lineZ);

  AddText(MeasureFormat::CircleRoi(m), m.center + QPointF(0, -m.radius - 16),
          styles.circleRoiColor);
}

void MeasureOverlayDrawer::DrawRectRoi(const domain::RectangularRoiMeasurement& m) {
  const QRectF rect = m.Rect();
  QGraphicsRectItem* rectItem = scene->addRect(
    rect, QPen(styles.rectRoiColor, styles.lineWidth), QBrush()
  );
  rectItem->setZValue(styles.lineZ);

  AddText(MeasureFormat::RectRoi(m), rect.topLeft() + QPointF(4, -14),
          styles.rectRoiColor);
}

namespace MeasureFormat {

QString Distance(const domain::DistanceMeasurement& m) {
  if (m.IsCalibrated()) {
    return QString::number(m.DistanceInMm(), 'f', 1) + UiStrings::UnitMm();
  }
  return QString::number(m.DistanceInPixels(), 'f', 0) + UiStrings::UnitPx();
}

QString Angle(const domain::AngleMeasurement& m) {
  return QString::number(m.AngleInDegrees(), 'f', 1) + UiStrings::UnitDegrees();
}

QString CircleRoi(const domain::CircularRoiMeasurement& m) {
  QString text;
  if (m.DiameterInMm() > 0.0) {
    text = QString("%1 %2%3")
      .arg(UiStrings::SymbolDiameter())
      .arg(m.DiameterInMm(), 0, 'f', 1)
      .arg(UiStrings::UnitMm());
  } else {
    text = QString("%1 %2%3")
      .arg(UiStrings::SymbolDiameter())
      .arg(2.0 * m.radius, 0, 'f', 0)
      .arg(UiStrings::UnitPx());
  }
  if (m.stats.pixelCount > 0) {
    text += QString("\n%1: %2  %3: %4")
      .arg(UiStrings::StatMean())
      .arg(m.stats.meanValue, 0, 'f', 0)
      .arg(UiStrings::StatStdDev())
      .arg(m.stats.stdDev, 0, 'f', 1);
  }
  return text;
}

QString RectRoi(const domain::RectangularRoiMeasurement& m) {
  QString text;
  if (m.WidthInMm() > 0.0 && m.HeightInMm() > 0.0) {
    text = QString("%1 × %2 %3")
      .arg(m.WidthInMm(), 0, 'f', 1)
      .arg(m.HeightInMm(), 0, 'f', 1)
      .arg(UiStrings::UnitMm());
  } else {
    const QRectF r = m.Rect();
    text = QString("%1 × %2 %3")
      .arg(r.width(), 0, 'f', 0)
      .arg(r.height(), 0, 'f', 0)
      .arg(UiStrings::UnitPx());
  }
  if (m.stats.pixelCount > 0) {
    text += QString("\n%1: %2  %3: %4")
      .arg(UiStrings::StatMean())
      .arg(m.stats.meanValue, 0, 'f', 0)
      .arg(UiStrings::StatStdDev())
      .arg(m.stats.stdDev, 0, 'f', 1);
  }
  return text;
}

}  // namespace MeasureFormat

}  // namespace presentation
