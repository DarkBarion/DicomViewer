#include "presentation/widgets/ImageConverter.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace presentation {

QImage ImageConverter::ToQImage(const domain::DicomImageData& data) {
  if (data.pixels.empty() || data.width <= 0 || data.height <= 0) {
    return {};
  }

  int stride = 0;
  QImage::Format format = QImage::Format_Grayscale8;
  switch (data.format) {
    case domain::PixelFormat::kGrayscale8:
      format = QImage::Format_Grayscale8;
      stride = data.width;
      break;
    case domain::PixelFormat::kRgb8:
      format = QImage::Format_RGB888;
      stride = data.width * 3;
      break;
    default:
      return {};
  }

  return QImage(data.pixels.data(), data.width, data.height, stride, format)
    .copy();
}

std::vector<std::uint8_t> ImageConverter::BuildWindowLut(
  double center,
  double widthVal,
  bool isSigmoid
) {
  std::vector<std::uint8_t> lut(65536);
  const double minValue = center - (widthVal / 2.0);

  for (std::size_t i = 0; i < 65536; ++i) {
    const double value = static_cast<double>(i);
    double normalized = 0.0;
    if (isSigmoid) {
      normalized = 1.0 / (1.0 + std::exp(-4.0 * (value - center) / widthVal));
    } else {
      normalized = (value - minValue) / widthVal;
    }
    lut[i] = static_cast<std::uint8_t>(
      std::lround(std::clamp(normalized, 0.0, 1.0) * 255.0)
    );
  }
  return lut;
}

void ImageConverter::ApplyWindowToBuffer(
  const std::vector<std::uint16_t>& rawPixels,
  std::uint8_t* destination,
  int width,
  int height,
  double center,
  double widthVal,
  bool isSigmoid
) {
  const std::vector<std::uint8_t> lut = BuildWindowLut(center, widthVal, isSigmoid);

  const std::size_t totalPixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  for (std::size_t i = 0; i < totalPixels; ++i) {
    destination[i] = lut[rawPixels[i]];
  }
}

QImage ImageConverter::ToQImageWithWindow(
  const domain::DicomImageData& data,
  const domain::WindowLevel& windowLevel
) {
  if (data.rawPixels.empty() || data.width <= 0 || data.height <= 0) {
    return ToQImage(data);
  }
  if (!windowLevel.IsValid()) {
    return ToQImage(data);
  }

  const double center = windowLevel.center;
  const double width = windowLevel.width;
  const bool isSigmoid =
    windowLevel.function == domain::VoiLutFunction::kSigmoid;

  const std::vector<std::uint8_t> lut = BuildWindowLut(center, width, isSigmoid);

  QImage image(data.width, data.height, QImage::Format_Grayscale8);
  for (int y = 0; y < data.height; ++y) {
    std::uint8_t* line = image.scanLine(y);
    const std::size_t rowOffset =
      static_cast<std::size_t>(y) *
      static_cast<std::size_t>(data.width);
    for (int x = 0; x < data.width; ++x) {
      const std::size_t index = rowOffset + static_cast<std::size_t>(x);
      line[x] = lut[data.rawPixels[index]];
    }
  }
  return image;
}

}  // namespace presentation