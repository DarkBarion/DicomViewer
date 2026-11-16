#pragma once

#include <array>
#include <string_view>

namespace infrastructure {

namespace LoaderStrings {

inline std::string ErrReadDicom() {
  return "Nao foi possivel ler o arquivo DICOM. Verifique se o arquivo e valido.";
}

inline std::string ErrInvalidDimensions() {
  return "Dimensoes de imagem invalidas.";
}

inline std::string ErrEmptyBuffer() {
  return "Buffer de pixels vazio.";
}

inline std::string ErrGetBuffer() {
  return "Falha ao obter o buffer de pixels.";
}

inline std::string LabelDefault() {
  return "Padrao";
}

inline std::array<std::string_view, 5> PresetLabels() {
  return {"Padrao", "Contraste", "Denso", "Janela 4", "Janela 5"};
}

inline constexpr std::string_view LogUserWl = "Applied user W/L: C={} W={}";
inline constexpr std::string_view LogVoiLut = "Applied VOI LUT: {} entries, {} bits";
inline constexpr std::string_view LogFileWc = "Applied file W/C: C={} W={} fn={}";
inline constexpr std::string_view LogFallbackMinMax = "No W/C or VOI LUT; fell back to min-max: C={} W={}";
inline constexpr std::string_view LogPixelSpacingMalformed = "PixelSpacing malformado — deixa em 0";

}  // namespace LoaderStrings

}  // namespace infrastructure
