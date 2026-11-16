#pragma once

#include <map>
#include <string>

namespace domain {

struct DicomMetadata {
  std::string patientName;
  std::string patientId;
  std::string studyDescription;
  std::string seriesDescription;
  std::string modality;
  std::string studyDate;
  std::string institutionName;
  std::map<std::string, std::string> extra;
};

}  // namespace domain