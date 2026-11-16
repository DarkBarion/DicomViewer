#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "DicomImageData.h"
#include "DicomMetadata.h"
#include "Result.h"

namespace domain {

struct LoadOptions {
  std::optional<WindowLevel> windowLevel;
  bool loadMetadata = true;
};

struct LoadedDicom {
  DicomImageData image;
  DicomMetadata metadata;
};

class IDicomLoader {
 public:
  virtual ~IDicomLoader() = default;

  virtual Result<LoadedDicom> Load(
    std::string_view filePath,
    const LoadOptions& options = {}
  ) = 0;

 protected:
  IDicomLoader() = default;
};

}  // namespace domain