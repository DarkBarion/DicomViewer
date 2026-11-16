#include "GdcmDicomLoader.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include <gdcmDataSet.h>
#include <gdcmImage.h>
#include <gdcmImageReader.h>
#include <gdcmPhotometricInterpretation.h>
#include <gdcmSequenceOfItems.h>
#include <gdcmTag.h>
#include <gdcmVR.h>

#include "logging/Logger.h"

using domain::DicomImageData;
using domain::DicomMetadata;
using domain::LoadedDicom;
using domain::LoadOptions;
using domain::PixelFormat;
using domain::Result;
using domain::ResultFail;
using domain::ResultOk;
using domain::VoiLutFunction;
using domain::WindowLevel;

namespace infrastructure {
namespace {

struct VoiLut {
  std::vector<std::uint16_t> data;
  std::int32_t firstMapped = 0;
  int bits = 8;
};

std::string GetString(
  const gdcm::DataSet& dataset,
  const gdcm::Tag& tag
) {
  if (!dataset.FindDataElement(tag)) return {};
  const gdcm::DataElement& element = dataset.GetDataElement(tag);
  if (element.IsEmpty()) return {};
  const gdcm::ByteValue* byteValue = element.GetByteValue();
  if (!byteValue) return {};
  return std::string(byteValue->GetPointer(), byteValue->GetLength());
}

bool IsMonochrome1(const gdcm::Image& image) {
  return image.GetPhotometricInterpretation() ==
         gdcm::PhotometricInterpretation::MONOCHROME1;
}

bool HasOverlayPlanes(const gdcm::DataSet& dataset) {
  for (unsigned short group = 0x6000; group <= 0x601E; group += 2) {
    if (dataset.FindDataElement(gdcm::Tag(group, 0x3000))) return true;
  }
  return false;
}

std::string GetTransferSyntaxUid(gdcm::ImageReader& reader) {
  if (!reader.GetFile().GetHeader().IsValid()) return "Unknown";
  const gdcm::FileMetaInformation& header = reader.GetFile().GetHeader();
  const gdcm::Tag tag(0x0002, 0x0010);
  if (!header.FindDataElement(tag)) return "Unknown";
  const gdcm::DataElement& element = header.GetDataElement(tag);
  const gdcm::ByteValue* byteValue = element.GetByteValue();
  if (!byteValue) return "Unknown";
  return std::string(byteValue->GetPointer(), byteValue->GetLength());
}

VoiLutFunction ParseVoiLutFunction(const gdcm::DataSet& dataset) {
  const std::string value = GetString(dataset, gdcm::Tag(0x0028, 0x1056));
  if (value == "SIGMOID") return VoiLutFunction::kSigmoid;
  if (value == "LINEAR_EXACT") return VoiLutFunction::kLinearExact;
  return VoiLutFunction::kLinear;
}

std::vector<std::string> SplitBackslash(const std::string& input) {
  std::vector<std::string> parts;
  std::size_t position = 0;
  while (position <= input.size()) {
    const std::string::size_type next = input.find('\\', position);
    if (next == std::string::npos) {
      parts.push_back(input.substr(position));
      break;
    }
    parts.push_back(input.substr(position, next - position));
    position = next + 1;
  }
  return parts;
}

std::vector<WindowLevel> TryReadWindowLevels(
  const gdcm::DataSet& dataset
) {
  std::vector<WindowLevel> result;

  const gdcm::Tag tagWindowCenter(0x0028, 0x1050);
  const gdcm::Tag tagWindowWidth(0x0028, 0x1051);
  if (!dataset.FindDataElement(tagWindowCenter)) return result;
  if (!dataset.FindDataElement(tagWindowWidth)) return result;

  const std::vector<std::string> centers = SplitBackslash(GetString(dataset, tagWindowCenter));
  const std::vector<std::string> widths = SplitBackslash(GetString(dataset, tagWindowWidth));
  const VoiLutFunction function = ParseVoiLutFunction(dataset);

  static const char* kLabels[] = {
    "Padrão", "Contraste", "Denso", "Janela 4", "Janela 5",
  };

  const std::size_t count = std::min(centers.size(), widths.size());
  for (std::size_t i = 0; i < count; ++i) {
    try {
      WindowLevel windowLevel;
      windowLevel.center = std::stod(centers[i]);
      windowLevel.width = std::stod(widths[i]);
      windowLevel.function = function;
      windowLevel.label = (count == 1) ? "Padrão" : kLabels[i < 5 ? i : 4];
      if (windowLevel.IsValid()) {
        result.push_back(std::move(windowLevel));
      }
    }
    catch (const std::exception&) {
      // Valor malformado — ignora esta entrada, mantém as válidas.
    }
  }
  return result;
}

bool TryReadVoiLut(
  const gdcm::DataSet& dataset,
  VoiLut* out
) {
  const gdcm::Tag tagSequence(0x0028, 0x3010);
  if (!dataset.FindDataElement(tagSequence)) return false;

  const gdcm::DataElement& sequenceElement = dataset.GetDataElement(tagSequence);
  const gdcm::SequenceOfItems* sequence = sequenceElement.GetValueAsSQ();
  if (!sequence || sequence->GetNumberOfItems() == 0) return false;

  const gdcm::Item& item = sequence->GetItem(1);

  const gdcm::Tag tagDescriptor(0x0028, 0x3002);
  if (!item.FindDataElement(tagDescriptor)) return false;
  const gdcm::DataElement& descriptorElement = item.GetDataElement(tagDescriptor);
  const gdcm::ByteValue* descriptorBytes = descriptorElement.GetByteValue();
  if (!descriptorBytes || descriptorBytes->GetLength() < 6) return false;

  const std::uint16_t* descriptor = reinterpret_cast<const std::uint16_t*>(
    descriptorBytes->GetPointer()
  );
  const std::uint16_t entries = descriptor[0];
  const gdcm::VR vr = descriptorElement.GetVR();
  const std::int32_t firstMapped =
    (vr == gdcm::VR::SS)
      ? static_cast<std::int16_t>(descriptor[1])
      : static_cast<std::int32_t>(descriptor[1]);
  const std::uint16_t bits = descriptor[2];
  if (bits != 8 && bits != 16) return false;

  const gdcm::Tag tagData(0x0028, 0x3006);
  if (!item.FindDataElement(tagData)) return false;
  const gdcm::ByteValue* dataBytes = item.GetDataElement(tagData).GetByteValue();
  if (!dataBytes) return false;

  const std::size_t numEntries =
    (entries == 0) ? 65536u : static_cast<std::size_t>(entries);
  const std::size_t expectedBytes = numEntries * (bits / 8);
  if (dataBytes->GetLength() < expectedBytes) return false;

  out->bits = bits;
  out->firstMapped = firstMapped;
  out->data.resize(numEntries);

  if (bits == 8) {
    const std::uint8_t* src = reinterpret_cast<const std::uint8_t*>(
      dataBytes->GetPointer()
    );
    std::copy(src, src + numEntries, out->data.begin());
  }
  else {
    const std::uint16_t* src = reinterpret_cast<const std::uint16_t*>(
      dataBytes->GetPointer()
    );
    std::copy(src, src + numEntries, out->data.begin());
  }
  return true;
}

void ApplyVoiLutToPixels(
  const std::vector<std::uint16_t>& raw,
  const VoiLut& voiLut,
  std::vector<std::uint8_t>* out
) {
  const std::size_t count = raw.size();
  out->resize(count);

  const std::int32_t first = voiLut.firstMapped;
  const std::int32_t last =
    first + static_cast<std::int32_t>(voiLut.data.size()) - 1;
  const double maxValue = (voiLut.bits == 8) ? 255.0 : 65535.0;

  for (std::size_t i = 0; i < count; ++i) {
    const std::int32_t value = static_cast<std::int32_t>(raw.at(i));
    std::int32_t index;
    if (value <= first) {
      index = 0;
    }
    else if (value >= last) {
      index = static_cast<std::int32_t>(voiLut.data.size()) - 1;
    }
    else {
      index = value - first;
    }
    const double normalized =
      static_cast<double>(voiLut.data.at(index)) / maxValue;
    (*out).at(i) = static_cast<std::uint8_t>(
      std::lround(std::clamp(normalized, 0.0, 1.0) * 255.0)
    );
  }
}

void ApplyWindowLevelToPixels(
  const std::vector<std::uint16_t>& raw,
  const WindowLevel& windowLevel,
  std::vector<std::uint8_t>* out
) {
  const std::size_t count = raw.size();
  out->resize(count);

  const double center = windowLevel.center;
  const double width = windowLevel.width;

  if (windowLevel.function == VoiLutFunction::kSigmoid) {
    for (std::size_t i = 0; i < count; ++i) {
      const double value = static_cast<double>(raw.at(i));
      const double normalized =
        1.0 / (1.0 + std::exp(-4.0 * (value - center) / width));
      (*out).at(i) = static_cast<std::uint8_t>(
        std::lround(std::clamp(normalized, 0.0, 1.0) * 255.0)
      );
    }
    return;
  }

  const double minValue = center - width / 2.0;
  for (std::size_t i = 0; i < count; ++i) {
    const double normalized =
      (static_cast<double>(raw.at(i)) - minValue) / width;
    (*out).at(i) = static_cast<std::uint8_t>(
      std::lround(std::clamp(normalized, 0.0, 1.0) * 255.0)
    );
  }
}

void TryReadPixelSpacing(
  const gdcm::DataSet& dataset,
  DicomImageData* data
) {
  const std::string spacing = GetString(dataset, gdcm::Tag(0x0028, 0x0030));
  const std::string::size_type separator = spacing.find('\\');
  if (separator != std::string::npos) {
    try {
      data->pixelSpacingRow = std::stod(spacing.substr(0, separator));
      data->pixelSpacingCol = std::stod(spacing.substr(separator + 1));
    }
    catch (const std::exception&) {
      // PixelSpacing malformado — deixa em 0.
    }
  }
}

void NormalizeByMinMax(
  const std::vector<std::uint16_t>& raw,
  std::vector<std::uint8_t>* out,
  WindowLevel* outWindowLevel
) {
  if (raw.empty()) {
    out->clear();
    return;
  }
  const auto [minIt, maxIt] = std::minmax_element(raw.begin(), raw.end());
  const double minValue = *minIt;
  const double maxValue = *maxIt;
  const double range = (maxValue > minValue) ? (maxValue - minValue) : 1.0;

  out->resize(raw.size());
  for (std::size_t i = 0; i < raw.size(); ++i) {
    const double normalized =
      (static_cast<double>(raw.at(i)) - minValue) / range;
    (*out).at(i) = static_cast<std::uint8_t>(
      std::lround(std::clamp(normalized, 0.0, 1.0) * 255.0)
    );
  }
  outWindowLevel->center = (minValue + maxValue) / 2.0;
  outWindowLevel->width = range;
}

}  // namespace

Result<LoadedDicom> GdcmDicomLoader::Load(
  std::string_view filePath,
  const LoadOptions& options
) {
  Logger::Info("Loading DICOM (GDCM): {}", std::string(filePath));

  gdcm::ImageReader reader;
  reader.SetFileName(std::string(filePath).c_str());
  if (!reader.Read()) {
    return ResultFail<LoadedDicom>(
      "Não foi possível ler o arquivo DICOM. "
      "Verifique se o arquivo é válido."
    );
  }

  Result<DicomImageData> imageResult = ExtractImageData(
    reader.GetImage(),
    reader.GetFile().GetDataSet(),
    options
  );
  if (!imageResult) {
    return ResultFail<LoadedDicom>(imageResult.error().message);
  }

  LoadedDicom loaded;
  loaded.image = std::move(imageResult).value();
  loaded.metadata = ExtractMetadata(reader.GetFile().GetDataSet());
  loaded.image.transferSyntaxUid = GetTransferSyntaxUid(reader);

  Logger::Info(
    "Loaded: {}x{} | TS={} | Mono1={} | Patient={}",
    loaded.image.width, loaded.image.height,
    loaded.image.transferSyntaxUid,
    loaded.image.isMonochrome1 ? "yes" : "no",
    Redact(loaded.metadata.patientName)
  );

  return ResultOk<LoadedDicom>(std::move(loaded));
}

Result<DicomImageData> GdcmDicomLoader::ExtractImageData(
  const gdcm::Image& image,
  const gdcm::DataSet& dataset,
  const LoadOptions& options
) {
  const unsigned int* dimensions = image.GetDimensions();
  const int width = static_cast<int>(dimensions[0]);
  const int height = static_cast<int>(dimensions[1]);

  if (width <= 0 || height <= 0) {
    return ResultFail<DicomImageData>("Dimensões de imagem inválidas.");
  }

  const unsigned long bufferLength = image.GetBufferLength();
  if (bufferLength == 0) {
    return ResultFail<DicomImageData>("Buffer de pixels vazio.");
  }

  std::vector<char> buffer(bufferLength);
  if (!image.GetBuffer(buffer.data())) {
    return ResultFail<DicomImageData>("Falha ao obter o buffer de pixels.");
  }

  DicomImageData data;
  data.width = width;
  data.height = height;
  data.isMonochrome1 = IsMonochrome1(image);
  data.hasOverlays = HasOverlayPlanes(dataset);

  const gdcm::PixelFormat& pixelFormat = image.GetPixelFormat();
  const bool isMonochrome = (pixelFormat.GetSamplesPerPixel() == 1);
  data.format = isMonochrome ? PixelFormat::kGrayscale8 : PixelFormat::kRgb8;

  const std::size_t pixelCount =
    static_cast<std::size_t>(width) * static_cast<std::size_t>(height);

  if (isMonochrome && pixelFormat.GetBitsAllocated() == 16) {
    data.rawPixels.resize(pixelCount);
    for (std::size_t i = 0; i < pixelCount; ++i) {
      data.rawPixels.at(i) =
        static_cast<std::uint16_t>(static_cast<std::uint8_t>(buffer.at(i * 2))) |
        (static_cast<std::uint16_t>(static_cast<std::uint8_t>(buffer.at(i * 2 + 1))) << 8);
    }

    data.availableWindowLevels = TryReadWindowLevels(dataset);
    VoiLut voiLut;
    data.hasVoiLut = TryReadVoiLut(dataset, &voiLut);

    const bool hasUserWindowLevel =
      options.windowLevel && options.windowLevel->IsValid();

    if (hasUserWindowLevel) {
      ApplyWindowLevelToPixels(
        data.rawPixels, *options.windowLevel, &data.pixels
      );
      data.appliedWindowLevel = *options.windowLevel;
      data.activeWindowLevelIndex = -1;
      Logger::Info(
        "Applied user W/L: C={} W={}",
        options.windowLevel->center, options.windowLevel->width
      );
    }
    else if (data.hasVoiLut) {
      ApplyVoiLutToPixels(data.rawPixels, voiLut, &data.pixels);
      data.voiLutPixels = data.pixels;
      data.voiLutApplied = true;
      data.activeWindowLevelIndex = -1;
      Logger::Info(
        "Applied VOI LUT: {} entries, {} bits",
        voiLut.data.size(), voiLut.bits
      );
    }
    else if (!data.availableWindowLevels.empty()) {
      const domain::WindowLevel& first = data.availableWindowLevels.front();
      ApplyWindowLevelToPixels(data.rawPixels, first, &data.pixels);
      data.appliedWindowLevel = first;
      data.activeWindowLevelIndex = 0;
      Logger::Info(
        "Applied file W/C: C={} W={} fn={}",
        first.center, first.width, std::to_underlying(first.function)
      );
    }
    else {
      WindowLevel windowLevel;
      NormalizeByMinMax(data.rawPixels, &data.pixels, &windowLevel);
      data.appliedWindowLevel = windowLevel;
      data.activeWindowLevelIndex = -1;
      Logger::Warn(
        "No W/C or VOI LUT; fell back to min-max: C={} W={}",
        windowLevel.center, windowLevel.width
      );
    }

    // PixelSpacing
    TryReadPixelSpacing(dataset, &data);

    return ResultOk<DicomImageData>(std::move(data));
  }

  data.pixels.resize(bufferLength);
  for (unsigned long i = 0; i < bufferLength; ++i) {
    data.pixels.at(i) = static_cast<std::uint8_t>(buffer.at(i));
  }

  TryReadPixelSpacing(dataset, &data);
  return ResultOk<DicomImageData>(std::move(data));
}

DicomMetadata GdcmDicomLoader::ExtractMetadata(
  const gdcm::DataSet& dataset
) {
  DicomMetadata metadata;
  metadata.patientName = GetString(dataset, gdcm::Tag(0x0010, 0x0010));
  metadata.patientId = GetString(dataset, gdcm::Tag(0x0010, 0x0020));
  metadata.studyDescription = GetString(dataset, gdcm::Tag(0x0008, 0x1030));
  metadata.seriesDescription = GetString(dataset, gdcm::Tag(0x0008, 0x103E));
  metadata.modality = GetString(dataset, gdcm::Tag(0x0008, 0x0060));
  metadata.studyDate = GetString(dataset, gdcm::Tag(0x0008, 0x0020));
  metadata.institutionName = GetString(dataset, gdcm::Tag(0x0008, 0x0080));
  return metadata;
}

}  // namespace infrastructure