#include "presentation/ImageConverter.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <utility>
#include <vector>

#include <QImage>

namespace presentation::test {

  using domain::DicomImageData;
  using domain::PixelFormat;
  using domain::VoiLutFunction;
  using domain::WindowLevel;

  namespace {

    DicomImageData MakeGrayscale(
      std::vector<std::uint8_t> newPixels,
      int newWidth,
      int newHeight
    ) {
      DicomImageData data;
      data.width = newWidth;
      data.height = newHeight;
      data.format = PixelFormat::kGrayscale8;
      data.pixels = std::move(newPixels);
      return data;
    }

  }  // namespace

  TEST(ImageConverter, EmptyDataReturnsNullImage) {
    DicomImageData empty;
    EXPECT_TRUE(ImageConverter::ToQImage(empty).isNull());
  }

  TEST(ImageConverter, ZeroDimensionsReturnsNull) {
    DicomImageData data;
    data.width = 0;
    data.height = 0;
    EXPECT_TRUE(ImageConverter::ToQImage(data).isNull());
  }

  TEST(ImageConverter, Grayscale8PreservesPixels) {
    auto data = MakeGrayscale({0, 64, 128, 255}, 2, 2);

    QImage image = ImageConverter::ToQImage(data);
    ASSERT_FALSE(image.isNull());
    EXPECT_EQ(image.width(), 2);
    EXPECT_EQ(image.height(), 2);
    EXPECT_EQ(image.format(), QImage::Format_Grayscale8);
    EXPECT_EQ(image.pixelColor(0, 0).red(), 0);
    EXPECT_EQ(image.pixelColor(1, 0).red(), 64);
    EXPECT_EQ(image.pixelColor(0, 1).red(), 128);
    EXPECT_EQ(image.pixelColor(1, 1).red(), 255);
  }

  TEST(ImageConverter, Rgb8PreservesChannels) {
    DicomImageData data;
    data.width = 1;
    data.height = 1;
    data.format = PixelFormat::kRgb8;
    data.pixels = {255, 128, 0};

    QImage image = ImageConverter::ToQImage(data);
    ASSERT_FALSE(image.isNull());
    const auto color = image.pixelColor(0, 0);
    EXPECT_EQ(color.red(), 255);
    EXPECT_EQ(color.green(), 128);
    EXPECT_EQ(color.blue(), 0);
  }

  TEST(ImageConverter, ResultIsDeepCopyNotReference) {
    auto data = MakeGrayscale({255}, 1, 1);

    QImage image = ImageConverter::ToQImage(data);
    ASSERT_FALSE(image.isNull());

    data.pixels[0] = 0;
    EXPECT_EQ(image.pixelColor(0, 0).red(), 255)
    << "QImage referencia buffer morto!";
  }

  TEST(ImageConverter, ToQImageWithWindowAppliesLinearLut) {
    DicomImageData data;
    data.width = 3;
    data.height = 1;
    data.format = PixelFormat::kGrayscale8;
    data.rawPixels = {0, 2048, 4096};
    data.pixels.resize(3, 0);

    WindowLevel windowLevel;
    windowLevel.center = 2048.0;
    windowLevel.width = 4096.0;
    windowLevel.function = VoiLutFunction::kLinear;

    QImage image = ImageConverter::ToQImageWithWindow(data, windowLevel);
    ASSERT_FALSE(image.isNull());
    EXPECT_EQ(image.pixelColor(0, 0).red(), 0);
    EXPECT_NEAR(image.pixelColor(1, 0).red(), 128, 1);
    EXPECT_EQ(image.pixelColor(2, 0).red(), 255);
  }

  TEST(ImageConverter, ToQImageWithWindowAppliesSigmoidLut) {
    DicomImageData data;
    data.width = 3;
    data.height = 1;
    data.format = PixelFormat::kGrayscale8;
    data.rawPixels = {0, 2048, 4096};
    data.pixels.resize(3, 0);

    WindowLevel windowLevel;
    windowLevel.center = 2048.0;
    windowLevel.width = 4096.0;
    windowLevel.function = VoiLutFunction::kSigmoid;

    QImage image = ImageConverter::ToQImageWithWindow(data, windowLevel);
    ASSERT_FALSE(image.isNull());
    // Sigmoid: 1/(1+exp(-4*(v-c)/w))
    //   v=0:    sigmoid(-2) ≈ 0.119 → ~30
    //   v=2048: sigmoid(0)  = 0.5   → 128
    //   v=4096: sigmoid(2)  ≈ 0.881 → ~225
    EXPECT_NEAR(image.pixelColor(0, 0).red(), 30, 2);
    EXPECT_NEAR(image.pixelColor(1, 0).red(), 128, 1);
    EXPECT_NEAR(image.pixelColor(2, 0).red(), 225, 2);
  }

  TEST(ImageConverter, ToQImageWithWindowClampsOutOfRange) {
    DicomImageData data;
    data.width = 2;
    data.height = 1;
    data.format = PixelFormat::kGrayscale8;
    data.rawPixels = {0, 1000};
    data.pixels.resize(2, 0);

    WindowLevel windowLevel;
    windowLevel.center = 500.0;
    windowLevel.width = 200.0;
    windowLevel.function = VoiLutFunction::kLinear;

    QImage image = ImageConverter::ToQImageWithWindow(data, windowLevel);
    EXPECT_EQ(image.pixelColor(0, 0).red(), 0);
    EXPECT_EQ(image.pixelColor(1, 0).red(), 255);
  }

  TEST(ImageConverter, ToQImageWithWindowFallsBackWhenNoRawPixels) {
    auto data = MakeGrayscale({128}, 1, 1);
    WindowLevel windowLevel{128.0, 256.0, VoiLutFunction::kLinear, ""};
    QImage image = ImageConverter::ToQImageWithWindow(data, windowLevel);
    ASSERT_FALSE(image.isNull());
    EXPECT_EQ(image.pixelColor(0, 0).red(), 128);
  }

}  // namespace presentation::test
