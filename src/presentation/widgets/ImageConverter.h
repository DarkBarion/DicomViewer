#pragma once

#include "domain/DicomImageData.h"

#include <QImage>
#include <vector>

namespace presentation {

class ImageConverter {
 public:
  static QImage ToQImage(const domain::DicomImageData& data);

  static QImage ToQImageWithWindow(
    const domain::DicomImageData& data,
    const domain::WindowLevel& windowLevel
  );

  static void ApplyWindowToBuffer(
    const std::vector<std::uint16_t>& rawPixels,
    std::uint8_t* destination,
    int width,
    int height,
    double center,
    double widthVal,
    bool isSigmoid = false
  );

  static std::vector<std::uint8_t> BuildWindowLut(
    double center,
    double widthVal,
    bool isSigmoid = false
  );
};

}  // namespace presentation