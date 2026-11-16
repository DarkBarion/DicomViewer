#pragma once

#include <QWidget>

class QComboBox;
class QTabWidget;
class QAction;

namespace presentation {

class RibbonBar : public QWidget {
  Q_OBJECT

 public:
  explicit RibbonBar(QWidget* parent = nullptr);

  QComboBox* windowPresetCombo() const { return m_presetCombo; }

  void SetMeasurementActionsEnabled(bool enabled);

  void SetNavigationActionsEnabled(bool enabled);

 signals:
  void OpenFileRequested();
  void CloseFileRequested();
  void CloseAllFilesRequested();

  void FitRequested();
  void Zoom11Requested();
  void FitContentRequested();

  void MeasureToggled(bool checked);
  void AngleToggled(bool checked);
  void CircleRoiToggled(bool checked);
  void RectRoiToggled(bool checked);
  void ClearMeasurementsRequested();

  void RotateCwRequested();
  void RotateCcwRequested();
  void FlipHRequested();
  void FlipVRequested();

  void WindowLevelToggled(bool checked);
  void WindowPresetChanged(int index);

 public slots:
  void PopulateWindowPresets(const QStringList& items, int activeIndex, bool enable);
  void SelectWindowPreset(int index);
  void UpdateCustomPresetText(const QString& text);

 private:
  void BuildTabs();
  void BuildExamineTab(QWidget* tab);
  void BuildMeasureTab(QWidget* tab);
  void BuildTransformTab(QWidget* tab);
  void BuildWindowTab(QWidget* tab);

  QWidget* BuildFileGroup();
  QWidget* BuildNavigationGroup();
  QWidget* BuildMeasureGroup();
  QWidget* BuildClearGroup();
  QWidget* BuildRotateGroup();
  QWidget* BuildFlipGroup();
  QWidget* BuildWlGroup();
  QWidget* BuildPresetGroup();

  QTabWidget* m_tabWidget = nullptr;
  QComboBox* m_presetCombo = nullptr;

  QAction* m_measureAction = nullptr;
  QAction* m_angleAction = nullptr;
  QAction* m_circleRoiAction = nullptr;
  QAction* m_rectRoiAction = nullptr;
  QAction* m_clearMeasuresAction = nullptr;

  QAction* m_fitAction = nullptr;
  QAction* m_zoom11Action = nullptr;
  QAction* m_fitContentAction = nullptr;

  QAction* m_rotateCwAction = nullptr;
  QAction* m_rotateCcwAction = nullptr;
  QAction* m_flipHAction = nullptr;
  QAction* m_flipVAction = nullptr;
};

}  // namespace presentation
