#pragma once

#include <compare>
#include <cmath>

#include <QPointF>

namespace domain {

class DistanceMeasurement {
 public:
  DistanceMeasurement(
    QPointF newStartPoint,
    QPointF newEndPoint,
    double newSpacingRow,
    double newSpacingCol
  ) :
    startPoint(newStartPoint),
    endPoint(newEndPoint),
    spacingRow(newSpacingRow),
    spacingCol(newSpacingCol)
  {
  }

  [[nodiscard]] double DistanceInPixels() const {
    const double dx = endPoint.x() - startPoint.x();
    const double dy = endPoint.y() - startPoint.y();
    return std::sqrt(dx * dx + dy * dy);
  }

  [[nodiscard]] bool IsCalibrated() const {
    return spacingRow > 0.0 && spacingCol > 0.0;
  }

  [[nodiscard]] double DistanceInMm() const {
    if (!IsCalibrated()) return 0.0;
    const double dx = (endPoint.x() - startPoint.x()) * spacingCol;
    const double dy = (endPoint.y() - startPoint.y()) * spacingRow;
    return std::sqrt(dx * dx + dy * dy);
  }

  [[nodiscard]] QPointF StartPoint() const {
    return startPoint;
  }

  [[nodiscard]] QPointF EndPoint() const {
    return endPoint;
  }

  auto operator<=>(const DistanceMeasurement&) const = default;

 private:
  QPointF startPoint;
  QPointF endPoint;
  double spacingRow;
  double spacingCol;
};

}  // namespace domain