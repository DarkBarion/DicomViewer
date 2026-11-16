#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

namespace domain {

struct HistogramData {
  std::vector<int> bins;
  int pixelMin = 0;
  int pixelMax = 65535;
};

class HistogramCalculator {
 public:
  static HistogramData Compute(
    const std::vector<std::uint16_t>& rawPixels,
    int numBins = 256
  ) {
    HistogramData data;
    const auto sizeBins = static_cast<std::size_t>(numBins);

    if (rawPixels.empty()) {
      data.bins.resize(sizeBins, 0);
      return data;
    }

    const auto [minIt, maxIt] = std::ranges::minmax_element(rawPixels);
    data.pixelMin = static_cast<int>(*minIt);
    data.pixelMax = static_cast<int>(*maxIt);

    data.bins.resize(sizeBins, 0);
    const int range = std::max(1, data.pixelMax - data.pixelMin);

    for (const std::uint16_t& value : rawPixels) {
      const int bin = std::min(
        numBins - 1,
        static_cast<int>(static_cast<double>(value - data.pixelMin) / range * (numBins - 1))
      );
      if (bin >= 0) {
        data.bins.at(static_cast<std::size_t>(bin))++;
      }
    }

    return data;
  }
};

}  // namespace domain
