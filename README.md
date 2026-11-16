# Dicom Viewer

Visualizador DICOM desktop em C++23, com Qt 6 e GDCM, projetado para o
contexto de medicina diagnóstica (telerradiologia / PACS).

## Bibliotecas

| Biblioteca | Versão mínima | Uso |
|---|---|---|
| Qt | 6.8 (testado com 6.12) | UI, i18n |
| GDCM | 3.0 | Parsing DICOM, decodificação de codecs (JPEG 2000 incluso) |
| CMake | 3.21 | Build |
| GoogleTest | 1.12 | Testes unitários |

## Estrutura

```
src/
├── domain/          # Tipos puros, sem dependências de Qt/GDCM
├── infrastructure/  # GdcmDicomLoader, Logger
├── presentation/    # ImageConverter, DicomViewerWidget, MainWindow, Presenter
├── application.h/cc # Ciclo de vida (Initialize/Run/Shutdown)
├── application_bootstrap.h/cc  # Composition root
└── main.cpp         # ~20 linhas, apenas cerimônia
```

A regra de dependência é: **presentation → domain ← infrastructure**. O
`main.cpp` só conhece `Application`, que só conhece `ApplicationBootstrap`,
que é o único lugar onde implementações concretas são escolhidas.

## Recursos C++23

O projeto utiliza recursos modernos do C++23 para código mais seguro e
expressivo:

- **`std::expected<T, Error>`** — substitui `Result<T>` customizado; oferece
  `.transform()`, `.value_or()` e captura automática de `std::stacktrace`
- **`std::to_underlying()`** — conversão segura de enum class para inteiro
- **`std::format`** — logging type-safe com `std::format_string` em tempo de
  compilação
- **`std::string_view`** — parâmetros de leitura sem cópia de string
- **Designated initializers** — inicialização nomeada em structs para evitar
  bugs de reordenação de campos
- **Three-way comparison `<=>`** — operadores de comparação gerados
  automaticamente
- **`[[nodiscard]]` com mensagem** — warnings explicativos do compilador
- **`constinit`** — garantia de inicialização estática em tempo de compilação
- **`std::optional<std::ofstream>`** — arquivo de log com inicialização lazy
  e destruição segura

## Compilação

### Pré-requisitos (CachyOS / Arch)

```bash
sudo pacman -S --needed base-devel cmake ninja git clang \
    qt6-base qt6-tools qt6-svg qt6-wayland gdcm gtest
```

### Build

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build --parallel
./build/DicomViewer
```

### Testes

```bash
cd build
ctest --output-on-failure
```

## Funcionalidades (Prioridade 1)

- Abrir e exibir arquivos DICOM monocromáticos e RGB
- Correções DICOM: Transfer Syntax com detecção específica, MONOCHROME1
  normalizado, `RescaleSlope`/`RescaleIntercept` via GDCM, `PixelSpacing`
  propagado, overlays detectados com aviso
- Window/Level interativo (botão direito + arrastar)
- Medição de distância em mm (calibrado) ou px (com aviso)
- Zoom (roda) e pan (arrastar)
- Painel de metadados com PHI (paciente, estudo, modalidade, spacing, W/L)
- i18n: pt-BR + EN via `tr()` e `QTranslator`
- LGPD: `Logger::Redact()` mascara PHI automaticamente no log
- Exit codes tipados; ciclo de vida explícito em `Application`

## DICOM Conformance Statement (resumo)

**SOP Classes suportadas (como SCU de leitura)**
- Secondary Capture Image Storage
- CT Image Storage
- MR Image Storage
- CR Image Storage
- DX Image Storage

**Transfer Syntaxes suportadas**
- Implicit VR Little Endian
- Explicit VR Little Endian
- Explicit VR Big Endian
- Deflated Explicit VR Little Endian
- RLE Lossless
- JPEG Baseline (Process 1)
- JPEG Lossless (Process 14)
- JPEG 2000 / JPEG-LS (via OpenJPEG compilado na GDCM do Arch)

**Media Storage**
- Leitura: DICOM Part 10 (file format)
- Escrita: N/A nesta versão

**Segurança**
- Sem DIMSE networking nesta versão
- Acesso a arquivo puramente local

## Licenciamento

- Código do projeto: **proprietário** (placeholder em `LICENSE`)
- Qt 6: **LGPL v3** — uso via dynamic linking, conforme exigido
- GDCM: **BSD-3-Clause** — compatível com uso comercial
- GoogleTest: **BSD-3-Clause**

## Roadmap (Prioridade 2)

- Multi-frame e cine
- MPR (axial/coronal/sagital), fusão PET-CT
- Medições persistentes (Presentation State)
- DICOM Web (QIDO-RS / WADO-RS)
- Cache de exames, worklist, hanging protocols
- Análise de risco ISO 14971 e matriz de rastreabilidade IEC 62304
- Build Windows/macOS com code signing e notarização
- `std::mdspan` para acesso multidimensional a pixels
- `std::source_location` no Logger para localização automática nos logs