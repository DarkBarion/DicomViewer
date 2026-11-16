#pragma once

#include <string_view>

#include "domain/IDicomLoader.h"

namespace gdcm {
class Image;
class DataSet;
}  // namespace gdcm

namespace infrastructure {

class GdcmDicomLoader final : public domain::IDicomLoader {
 public:
  domain::Result<domain::LoadedDicom> Load(
    std::string_view filePath,
    const domain::LoadOptions& options = {}
  ) override;

 private:
  domain::Result<domain::DicomImageData> ExtractImageData(
    const gdcm::Image& image,
    const gdcm::DataSet& dataset,
    const domain::LoadOptions& options
  );

  domain::DicomMetadata ExtractMetadata(const gdcm::DataSet& dataset);
};

}  // namespace infrastructure