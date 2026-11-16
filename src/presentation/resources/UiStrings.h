#pragma once

#include <QString>

namespace UiStrings {

inline QString ErrStartTitle() {
  return QObject::tr("Erro ao iniciar");
}

inline QString ErrStartBody(const QString& error) {
  return QObject::tr("Não foi possível iniciar a aplicação:\n%1").arg(error);
}

inline QString AppTitle() {
  return QObject::tr("Visualizador DICOM");
}

inline QString NoExamLoaded() {
  return QObject::tr("Nenhum exame carregado");
}

inline QString Ready() {
  return QObject::tr("Pronto");
}

inline QString MenuFile() {
  return QObject::tr("&Arquivo");
}

inline QString MenuOpen() {
  return QObject::tr("&Abrir DICOM...");
}

inline QString MenuQuit() {
  return QObject::tr("Sair");
}

inline QString ToolbarTools() {
  return QObject::tr("Ferramentas");
}

inline QString ToolbarOpen() {
  return QObject::tr("Abrir");
}

inline QString ToolbarClose() {
  return QObject::tr("Fechar");
}

inline QString ToolbarCloseTooltip() {
  return QObject::tr("Fechar arquivo atual (Ctrl+W)");
}

inline QString ToolbarFit() {
  return QObject::tr("Ajustar");
}

inline QString ToolbarFitTooltip() {
  return QObject::tr("Ajustar imagem à janela (Ctrl+0)");
}

inline QString ToolbarMeasure() {
  return QObject::tr("Medir");
}

inline QString ToolbarClearMeasures() {
  return QObject::tr("Limpar medidas");
}

inline QString ToolbarAngle() {
  return QObject::tr("Ângulo");
}

inline QString ToolbarCircleRoi() {
  return QObject::tr("ROI Circular");
}

inline QString ToolbarRectRoi() {
  return QObject::tr("ROI Retangular");
}

inline QString ToolbarRotateCw() {
  return QObject::tr("Rotacionar →");
}

inline QString ToolbarRotateCwTooltip() {
  return QObject::tr("Rotacionar 90° horário (R)");
}

inline QString ToolbarRotateCcw() {
  return QObject::tr("Rotacionar ←");
}

inline QString ToolbarRotateCcwTooltip() {
  return QObject::tr("Rotacionar 90° anti-horário (Shift+R)");
}

inline QString ToolbarFlipH() {
  return QObject::tr("Espelhar H");
}

inline QString ToolbarFlipHTooltip() {
  return QObject::tr("Espelhar horizontalmente (H)");
}

inline QString ToolbarFlipV() {
  return QObject::tr("Espelhar V");
}

inline QString ToolbarFlipVTooltip() {
  return QObject::tr("Espelhar verticalmente (V)");
}

inline QString ToolbarZoom11() {
  return QObject::tr("1:1");
}

inline QString ToolbarZoom11Tooltip() {
  return QObject::tr("Zoom 1:1 pixel real (Ctrl+1)");
}

inline QString ToolbarFitContent() {
  return QObject::tr("Ajustar conteúdo");
}

inline QString ToolbarFitContentTooltip() {
  return QObject::tr("Recortar fundo preto e ajustar (Ctrl+F)");
}

inline QString ToolbarPresetLabel() {
  return QObject::tr(" Preset: ");
}

inline QString ToolbarWindowLevel() {
  return QObject::tr("Window/Level");
}

inline QString ToolbarAdjustWl() {
  return QObject::tr("Ajustar W/L");
}

inline QString ToolbarAdjustWlTooltip() {
  return QObject::tr("Arraste na imagem para ajustar brilho/contraste (W)");
}

inline QString ToolbarWlHint() {
  return QObject::tr("  Botão do meio + arrastar: W/L  |  Histograma: handles arrastáveis");
}

inline QString DialogOpenTitle() {
  return QObject::tr("Selecionar arquivo DICOM");
}

inline QString DialogOpenFilter() {
  return QObject::tr("DICOM (*.dcm *.DCM);;Todos (*)");
}

inline QString ErrLoadDicomTitle() {
  return QObject::tr("Erro ao carregar DICOM");
}

inline QString ErrConvertImage() {
  return QObject::tr("Não foi possível converter a imagem para exibição.");
}

inline QString WarningMonochrome1() {
  return QObject::tr("Aviso: imagem MONOCHROME1 (interpretação de escala invertida "
                      "conforme padrão DICOM).");
}

inline QString WarningOverlays() {
  return QObject::tr("Aviso: arquivo contém overlays DICOM não exibidos.");
}

inline QString PresetText(const QString& label, double center, double width) {
  return QObject::tr("%1: C=%2 W=%3")
             .arg(label)
             .arg(center, 0, 'f', 0)
             .arg(width, 0, 'f', 0);
}

inline QString PresetSigmoidSuffix() {
  return QObject::tr(" (sigmoide)");
}

inline QString CustomPresetText(double center, double width) {
  return QObject::tr("Customizado: C=%1 W=%2")
             .arg(center, 0, 'f', 0)
             .arg(width, 0, 'f', 0);
}

inline QString VoiLutPreset() {
  return QObject::tr("VOI LUT (fabricante)");
}

inline QString CursorPosition(int x, int y, int value) {
  return QObject::tr("X: %1  Y: %2  Valor: %3").arg(x).arg(y).arg(value);
}

inline QString PatientInfo(const QString& name) {
  return QObject::tr("Paciente: %1").arg(name);
}

inline QString StudyInfo(const QString& description) {
  return QObject::tr("Estudo: %1").arg(description);
}

inline QString ModalityInfo(const QString& modality) {
  return QObject::tr("Modalidade: %1").arg(modality);
}

inline QString ImageDimensions(int width, int height) {
  return QObject::tr("%1×%2").arg(width).arg(height);
}

inline QString PixelSpacing(double row, double col) {
  return QObject::tr("Spacing: %1×%2 mm")
             .arg(row, 0, 'f', 3)
             .arg(col, 0, 'f', 3);
}

inline QString PixelSpacingUnknown() {
  return QObject::tr("Spacing: não informado (medição em px)");
}

inline QString VoiLutStatus() {
  return QObject::tr("VOI LUT (fabricante)");
}

inline QString WindowLevelStatus(double center, double width) {
  return QObject::tr("W/L: %1/%2")
             .arg(center, 0, 'f', 0)
             .arg(width, 0, 'f', 0);
}

inline QString RibbonExamine() {
  return QObject::tr("Examinar");
}

inline QString RibbonMeasure() {
  return QObject::tr("Medir");
}

inline QString RibbonTransform() {
  return QObject::tr("Transformar");
}

inline QString RibbonWindow() {
  return QObject::tr("Janela");
}

inline QString RibbonFileGroup() {
  return QObject::tr("Arquivo");
}

inline QString NoFilesOpen() {
  return QObject::tr("Nenhum arquivo aberto");
}

inline QString NoFilesOpenHint() {
  return QObject::tr("Use Abrir (Ctrl+O) para carregar um arquivo DICOM");
}

inline QString MenuClose() {
  return QObject::tr("Fe&char...");
}

inline QString MenuCloseAll() {
  return QObject::tr("Fechar &tudo");
}

inline QString RibbonNavigationGroup() {
  return QObject::tr("Navegação");
}

inline QString RibbonMeasureGroup() {
  return QObject::tr("Ferramentas");
}

inline QString RibbonClearGroup() {
  return QObject::tr("Limpar");
}

inline QString RibbonRotateGroup() {
  return QObject::tr("Rotação");
}

inline QString RibbonFlipGroup() {
  return QObject::tr("Espelhamento");
}

inline QString RibbonWlGroup() {
  return QObject::tr("Ajuste");
}

inline QString RibbonPresetGroup() {
  return QObject::tr("Presets");
}

inline QString UnitMm() {
  return QObject::tr(" mm");
}

inline QString UnitPx() {
  return QObject::tr(" px");
}

inline QString UnitDegrees() {
  return QObject::tr("°");
}

inline QString SymbolDiameter() {
  return QObject::tr("Ø");
}

inline QString StatMean() {
  return QObject::tr("Média");
}

inline QString StatStdDev() {
  return QObject::tr("Desv");
}

inline QString MetaInfoSeparator() {
  return QStringLiteral("   •   ");
}

inline QString ZoomInitial() {
  return QObject::tr("100%");
}

}  // namespace UiStrings
