cmake_minimum_required(VERSION 3.21)
project(DicomViewer LANGUAGES CXX VERSION 0.1.0)

# ═══════════════════════════════════════════════════════════════════════
# Padrões de C++ e CMake
# ═══════════════════════════════════════════════════════════════════════
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ═══════════════════════════════════════════════════════════════════════
# Opções
# ═══════════════════════════════════════════════════════════════════════
option(BUILD_TESTS       "Build unit tests"                OFF)
option(ENABLE_WARNINGS   "Enable strict warnings on our code" ON)
option(ENABLE_SANITIZERS "Enable ASAN/UBSAN"                OFF)
option(ENABLE_COVERAGE   "Enable coverage instrumentation"  OFF)

# ═══════════════════════════════════════════════════════════════════════
# Warnings — aplicados apenas ao NOSSO código, nunca a headers de terceiros
# ═══════════════════════════════════════════════════════════════════════
if(ENABLE_WARNINGS)
  if(MSVC)
    add_compile_options(/W4 /permissive-)
  else()
    add_compile_options(
      -Wall -Wextra -Wpedantic -Wshadow
      -Wnon-virtual-dtor -Wold-style-cast -Wcast-align
      -Wunused -Woverloaded-virtual
      -Wconversion -Wsign-conversion -Wdouble-promotion
    )
  endif()
endif()

# ═══════════════════════════════════════════════════════════════════════
# Sanitizers
# ═══════════════════════════════════════════════════════════════════════
if(ENABLE_SANITIZERS AND NOT MSVC)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=address,undefined)
endif()

# ═══════════════════════════════════════════════════════════════════════
# Coverage
# ═══════════════════════════════════════════════════════════════════════
if(ENABLE_COVERAGE AND NOT MSVC)
  add_compile_options(--coverage -O0 -g)
  add_link_options(--coverage)
endif()

# ═══════════════════════════════════════════════════════════════════════
# Dependências
# ═══════════════════════════════════════════════════════════════════════
find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Gui LinguistTools)
find_package(GDCM REQUIRED)

# ═══════════════════════════════════════════════════════════════════════
# Camada de Domínio (header-only)
# ═══════════════════════════════════════════════════════════════════════
# Contém: Result.h, DicomImageData.h, DicomMetadata.h, IDicomLoader.h,
#         Measurement.h, ILogger.h
# Sem dependências externas — apenas C++ puro.
add_library(dicom_domain INTERFACE)
target_include_directories(dicom_domain INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

# ═══════════════════════════════════════════════════════════════════════
# Camada de Infraestrutura
# ═══════════════════════════════════════════════════════════════════════
# Adaptadores concretos: GDCM (DICOM), Logger (logging estático) e
# LoggerAdapter (ponte entre infrastructure::Logger e domain::ILogger).
# Único target do projeto que conhece GDCM.
add_library(dicom_infra
    src/infrastructure/GdcmDicomLoader.cpp
    src/infrastructure/logging/Logger.cpp
    src/infrastructure/logging/LoggerAdapter.cpp
)

target_link_libraries(dicom_infra
    PUBLIC  dicom_domain
    PRIVATE gdcmMSFF
)

# Headers públicos da infraestrutura ficam visíveis para quem linka
# dicom_infra (o ApplicationBootstrap). Permite `#include
# "infrastructure/GdcmDicomLoader.h"` sem caminho relativo.
target_include_directories(dicom_infra PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

# GDCM no Arch/CachyOS instala headers em /usr/include/gdcm-3.2.
# find_package(GDCM) NÃO propaga esse caminho automaticamente — precisamos
# adicioná-lo manualmente ao nosso target.
if(GDCM_INCLUDE_DIRS)
  target_include_directories(dicom_infra PRIVATE ${GDCM_INCLUDE_DIRS})
endif()

# Suprime warnings de terceiros nos headers da GDCM. NÃO usar
# -Wno-* global (mascararia problemas do nosso código) — só neste target.
if(NOT MSVC)
  target_compile_options(dicom_infra PRIVATE
      -Wno-conversion
      -Wno-sign-conversion
      -Wno-old-style-cast
  )
endif()

# ═══════════════════════════════════════════════════════════════════════
# Camada de Apresentação — biblioteca compartilhada
# ═══════════════════════════════════════════════════════════════════════
# ImageConverter é usado tanto pelo executável quanto pelos testes
# (ImageConverterTests). Extrair para uma biblioteca evita compilação
# duplicada e resolve o erro de linker "referência não definida".
add_library(dicom_presentation
    src/presentation/ImageConverter.cpp
)

target_link_libraries(dicom_presentation
    PUBLIC  dicom_domain
    PRIVATE Qt6::Gui
)

target_include_directories(dicom_presentation PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

# ═══════════════════════════════════════════════════════════════════════
# Executável principal
# ═══════════════════════════════════════════════════════════════════════
add_executable(DicomViewer
    src/main.cpp
    src/Application.cpp
    src/ApplicationBootstrap.cpp
    src/presentation/DicomViewerWidget.cpp
    src/presentation/HistogramLutBar.cpp
    src/presentation/MainWindow.cpp
    src/presentation/DicomViewerPresenter.cpp
    # ImageConverter.cpp NÃO entra aqui — vem via dicom_presentation
)

target_link_libraries(DicomViewer PRIVATE
    Qt6::Widgets
    dicom_infra
    dicom_presentation
    stdc++exp
)

# ═══════════════════════════════════════════════════════════════════════
# i18n
# ═══════════════════════════════════════════════════════════════════════
qt_add_translations(DicomViewer
    TS_FILES translations/DicomViewer_pt_BR.ts
    RESOURCE_PREFIX "/i18n"
)

# ═══════════════════════════════════════════════════════════════════════
# Testes
# ═══════════════════════════════════════════════════════════════════════
if(BUILD_TESTS)
  enable_testing()
  find_package(GTest REQUIRED)

  # Testes de domínio — não precisam de Qt.
  add_executable(domain_tests
      tests/domain/ResultTests.cpp
  )
  target_link_libraries(domain_tests PRIVATE
      dicom_domain
      GTest::gtest_main
      stdc++exp
  )

  # Testes de apresentação — precisam de Qt::Gui (QImage) e precisam
  # linkar dicom_presentation (que contém ImageConverter.cpp).
  add_executable(presentation_tests
      tests/presentation/ImageConverterTests.cpp
  )
  target_link_libraries(presentation_tests PRIVATE
      dicom_presentation
      Qt6::Gui
      GTest::gtest_main
  )

  include(GoogleTest)
  gtest_discover_tests(domain_tests)
  gtest_discover_tests(presentation_tests)
endif()
