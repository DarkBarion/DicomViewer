#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace domain {

enum class PixelFormat : std::uint8_t {
  kGrayscale8,
  kRgb8,
};

enum class VoiLutFunction : std::uint8_t {
  kLinear,
  kLinearExact,
  kSigmoid,
};

struct WindowLevel {
  double center = 0.0;
  double width = 0.0;
  VoiLutFunction function = VoiLutFunction::kLinear;
  std::string label;

  static WindowLevel MakeDefault() {
    return {.center = 128.0, .width = 256.0, .function = VoiLutFunction::kLinear, .label = {}};
  }

  [[nodiscard]] bool IsValid() const noexcept {
    return width > 0.0;
  }

  auto operator<=>(const WindowLevel&) const = default;
};

struct DicomImageData {
  int width = 0;
  int height = 0;
  PixelFormat format = PixelFormat::kGrayscale8;

  std::vector<std::uint8_t> pixels;
  std::vector<std::uint16_t> rawPixels;
  std::vector<std::uint8_t> voiLutPixels;

  double pixelSpacingRow = 0.0;
  double pixelSpacingCol = 0.0;

  std::optional<WindowLevel> appliedWindowLevel;
  std::vector<WindowLevel> availableWindowLevels;
  int activeWindowLevelIndex = -1;

  bool voiLutApplied = false;
  bool hasVoiLut = false;
  bool isMonochrome1 = false;
  bool hasOverlays = false;
  std::string transferSyntaxUid;

  [[nodiscard]] bool HasPixelSpacing() const noexcept {
    return pixelSpacingRow > 0.0 && pixelSpacingCol > 0.0;
  }
};

}  // namespace domain
