#include "domain/Measurement.h"

#include <QPolygonF>

namespace domain {

namespace {

PixelStatistics ComputeStatistics(
  const std::vector<std::uint16_t>& rawPixels,
  int imageWidth,
  int xStart, int xEnd, int yStart, int yEnd,
  const std::function<bool(int, int)>& contains
) {
  if (rawPixels.empty()) return {};

  double sum = 0.0;
  double minVal = static_cast<double>(rawPixels[0]);
  double maxVal = minVal;
  int count = 0;

  for (int y = yStart; y <= yEnd; ++y) {
    for (int x = xStart; x <= xEnd; ++x) {
      if (!contains(x, y)) continue;
      const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(imageWidth)
                              + static_cast<std::size_t>(x);
      const double val = static_cast<double>(rawPixels[index]);
      sum += val;
      minVal = std::min(minVal, val);
      maxVal = std::max(maxVal, val);
      ++count;
    }
  }

  if (count == 0) return {};

  PixelStatistics result;
  result.pixelCount = count;
  result.minValue = minVal;
  result.maxValue = maxVal;
  result.meanValue = sum / count;

  double sumSq = 0.0;
  const double mean = result.meanValue;
  for (int y = yStart; y <= yEnd; ++y) {
    for (int x = xStart; x <= xEnd; ++x) {
      if (!contains(x, y)) continue;
      const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(imageWidth)
                              + static_cast<std::size_t>(x);
      const double diff = static_cast<double>(rawPixels[index]) - mean;
      sumSq += diff * diff;
    }
  }
  result.stdDev = std::sqrt(sumSq / count);
  return result;
}

}  // namespace

PixelStatistics ComputeRoiStatistics(
  const std::vector<std::uint16_t>& rawPixels,
  int imageWidth, int imageHeight,
  const QPolygonF& polygon
) {
  if (rawPixels.empty() || imageWidth <= 0 || imageHeight <= 0) {
    return {};
  }

  const QRectF bounds = polygon.boundingRect();
  const int xStart = std::max(0, static_cast<int>(std::floor(bounds.left())));
  const int xEnd = std::min(imageWidth - 1, static_cast<int>(std::ceil(bounds.right())));
  const int yStart = std::max(0, static_cast<int>(std::floor(bounds.top())));
  const int yEnd = std::min(imageHeight - 1, static_cast<int>(std::ceil(bounds.bottom())));

  if (xStart > xEnd || yStart > yEnd) return {};

  auto contains = [&polygon](int x, int y) {
    return polygon.containsPoint(QPointF(static_cast<double>(x), static_cast<double>(y)),
                                 Qt::WindingFill);
  };

  return ComputeStatistics(rawPixels, imageWidth, xStart, xEnd, yStart, yEnd, contains);
}

PixelStatistics ComputeCircularRoiStatistics(
  const std::vector<std::uint16_t>& rawPixels,
  int imageWidth, int imageHeight,
  QPointF center, double radius
) {
  if (rawPixels.empty() || imageWidth <= 0 || imageHeight <= 0 || radius <= 0.0) {
    return {};
  }

  const double radiusSq = radius * radius;
  const int xStart = std::max(0, static_cast<int>(std::floor(center.x() - radius)));
  const int xEnd = std::min(imageWidth - 1, static_cast<int>(std::ceil(center.x() + radius)));
  const int yStart = std::max(0, static_cast<int>(std::floor(center.y() - radius)));
  const int yEnd = std::min(imageHeight - 1, static_cast<int>(std::ceil(center.y() + radius)));

  if (xStart > xEnd || yStart > yEnd) return {};

  auto contains = [center, radiusSq](int x, int y) {
    const double dx = static_cast<double>(x) - center.x();
    const double dy = static_cast<double>(y) - center.y();
    return dx * dx + dy * dy <= radiusSq;
  };

  return ComputeStatistics(rawPixels, imageWidth, xStart, xEnd, yStart, yEnd, contains);
}

PixelStatistics ComputeRectangularRoiStatistics(
  const std::vector<std::uint16_t>& rawPixels,
  int imageWidth, int imageHeight,
  const QRectF& rect
) {
  if (rawPixels.empty() || imageWidth <= 0 || imageHeight <= 0) {
    return {};
  }

  const QRectF normalized = rect.normalized();
  const int xStart = std::max(0, static_cast<int>(std::floor(normalized.left())));
  const int xEnd = std::min(imageWidth - 1, static_cast<int>(std::ceil(normalized.right())));
  const int yStart = std::max(0, static_cast<int>(std::floor(normalized.top())));
  const int yEnd = std::min(imageHeight - 1, static_cast<int>(std::ceil(normalized.bottom())));

  if (xStart > xEnd || yStart > yEnd) {
    return {};
  }

  auto contains = [](int, int) { return true; };

  return ComputeStatistics(rawPixels, imageWidth, xStart, xEnd, yStart, yEnd, contains);
}

}  // namespace domain
