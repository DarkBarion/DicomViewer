#include "presentation/ui/RibbonBar.h"

#include <QAction>
#include <QComboBox>
#include <QFont>
#include <QFontDatabase>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPalette>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include "presentation/resources/UiIcons.h"
#include "presentation/resources/UiStrings.h"

namespace presentation {

namespace RibbonStyles {

inline QColor Background()    { return QColor(0x2d, 0x2d, 0x2d); }
inline QColor TabBg()         { return QColor(0x38, 0x38, 0x38); }
inline QColor TabSelected()   { return QColor(0x45, 0x45, 0x45); }
inline QColor GroupBg()       { return QColor(0x35, 0x35, 0x35); }
inline QColor GroupBorder()   { return QColor(0x50, 0x50, 0x50); }
inline QColor TextColor()     { return QColor(0xcc, 0xcc, 0xcc); }
inline QColor DimText()       { return QColor(0x88, 0x88, 0x88); }
inline QColor Accent()        { return QColor(0x56, 0x9c, 0xd6); }
inline QColor ButtonBg()      { return QColor(0x40, 0x40, 0x40); }
inline QColor ButtonHover()   { return QColor(0x50, 0x50, 0x50); }
inline QColor ButtonChecked() { return QColor(0x56, 0x9c, 0xd6); }

}  // namespace RibbonStyles

class ToolButton : public QToolButton {
  Q_OBJECT
 public:
  explicit ToolButton(const QIcon& icon, const QString& text, QWidget* parent = nullptr)
    : QToolButton(parent)
  {
    setIcon(icon);
    setText(text);
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    setIconSize(QSize(24, 24));
    setCheckable(false);
    setMinimumSize(60, 56);
    setMaximumSize(80, 64);
  }
};

class RibbonGroup : public QGroupBox {
  Q_OBJECT
 public:
  explicit RibbonGroup(const QString& title, QWidget* parent = nullptr)
    : QGroupBox(title, parent)
  {
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, RibbonStyles::GroupBg());
    pal.setColor(QPalette::WindowText, RibbonStyles::TextColor());
    setPalette(pal);
    setFlat(false);
    setContentsMargins(8, 20, 8, 8);
    QFont titleFont = font();
    titleFont.setPointSize(10);
    setFont(titleFont);
  }
};

RibbonBar::RibbonBar(QWidget* parent)
  : QWidget(parent)
{
  setAutoFillBackground(true);
  QPalette pal = palette();
  pal.setColor(QPalette::Window, RibbonStyles::Background());
  setPalette(pal);

  BuildTabs();
}

void RibbonBar::BuildTabs() {
  m_tabWidget = new QTabWidget(this);
  m_tabWidget->setDocumentMode(true);
  m_tabWidget->setTabsClosable(false);
  m_tabWidget->setMinimumHeight(100);
  m_tabWidget->setMaximumHeight(130);

  {
    QPalette tabPal = m_tabWidget->palette();
    tabPal.setColor(QPalette::Window, RibbonStyles::Background());
    tabPal.setColor(QPalette::WindowText, RibbonStyles::TextColor());
    tabPal.setColor(QPalette::BrightText, QColor(0xff, 0xff, 0xff));
    m_tabWidget->setPalette(tabPal);
  }

  QWidget* examineTab = new QWidget();
  BuildExamineTab(examineTab);
  m_tabWidget->addTab(examineTab, UiStrings::RibbonExamine());

  QWidget* measureTab = new QWidget();
  BuildMeasureTab(measureTab);
  m_tabWidget->addTab(measureTab, UiStrings::RibbonMeasure());

  QWidget* transformTab = new QWidget();
  BuildTransformTab(transformTab);
  m_tabWidget->addTab(transformTab, UiStrings::RibbonTransform());

  QWidget* windowTab = new QWidget();
  BuildWindowTab(windowTab);
  m_tabWidget->addTab(windowTab, UiStrings::RibbonWindow());

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(4, 0, 4, 0);
  layout->setSpacing(0);
  layout->addWidget(m_tabWidget);
}

void RibbonBar::BuildExamineTab(QWidget* tab) {
  QHBoxLayout* layout = new QHBoxLayout(tab);
  layout->setContentsMargins(8, 4, 8, 4);
  layout->setSpacing(12);
  layout->addWidget(BuildFileGroup());
  layout->addWidget(BuildNavigationGroup());
  layout->addStretch();
}

void RibbonBar::BuildMeasureTab(QWidget* tab) {
  QHBoxLayout* layout = new QHBoxLayout(tab);
  layout->setContentsMargins(8, 4, 8, 4);
  layout->setSpacing(12);
  layout->addWidget(BuildMeasureGroup());
  layout->addWidget(BuildClearGroup());
  layout->addStretch();
}

void RibbonBar::BuildTransformTab(QWidget* tab) {
  QHBoxLayout* layout = new QHBoxLayout(tab);
  layout->setContentsMargins(8, 4, 8, 4);
  layout->setSpacing(12);
  layout->addWidget(BuildRotateGroup());
  layout->addWidget(BuildFlipGroup());
  layout->addStretch();
}

void RibbonBar::BuildWindowTab(QWidget* tab) {
  QHBoxLayout* layout = new QHBoxLayout(tab);
  layout->setContentsMargins(8, 4, 8, 4);
  layout->setSpacing(12);
  layout->addWidget(BuildWlGroup());
  layout->addWidget(BuildPresetGroup());

  {
    QLabel* hint = new QLabel(UiStrings::ToolbarWlHint(), this);
    QPalette hintPal = hint->palette();
    hintPal.setColor(QPalette::WindowText, QColor(0x66, 0x66, 0x66));
    hint->setPalette(hintPal);
    QFont hintFont;
    hintFont.setPointSize(10);
    hint->setFont(hintFont);
    hint->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(hint);
  }

  layout->addStretch();
}

QWidget* RibbonBar::BuildFileGroup() {
  RibbonGroup* group = new RibbonGroup(UiStrings::RibbonFileGroup());
  QHBoxLayout* gl = new QHBoxLayout(group);
  gl->setSpacing(4);

  QAction* openAction = new QAction(UiIcons::Open(), UiStrings::ToolbarOpen(), this);
  connect(openAction, &QAction::triggered, this, &RibbonBar::OpenFileRequested);

  QToolButton* openBtn = new QToolButton(this);
  openBtn->setDefaultAction(openAction);
  openBtn->setIconSize(QSize(24, 24));
  openBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  openBtn->setMinimumHeight(36);
  gl->addWidget(openBtn);

  QAction* closeAction = new QAction(UiIcons::Close(), UiStrings::ToolbarClose(), this);
  closeAction->setShortcut(QKeySequence("Ctrl+W"));
  closeAction->setToolTip(UiStrings::ToolbarCloseTooltip());
  connect(closeAction, &QAction::triggered, this, &RibbonBar::CloseFileRequested);

  QToolButton* closeBtn = new QToolButton(this);
  closeBtn->setDefaultAction(closeAction);
  closeBtn->setIconSize(QSize(24, 24));
  closeBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  closeBtn->setMinimumHeight(36);
  gl->addWidget(closeBtn);

  return group;
}

QWidget* RibbonBar::BuildNavigationGroup() {
  RibbonGroup* group = new RibbonGroup(UiStrings::RibbonNavigationGroup());
  QHBoxLayout* gl = new QHBoxLayout(group);
  gl->setSpacing(4);

  m_fitAction = new QAction(UiIcons::Fit(), UiStrings::ToolbarFit(), this);
  m_fitAction->setShortcut(QKeySequence("Ctrl+0"));
  m_fitAction->setToolTip(UiStrings::ToolbarFitTooltip());
  connect(m_fitAction, &QAction::triggered, this, &RibbonBar::FitRequested);

  m_zoom11Action = new QAction(UiIcons::Zoom11(), UiStrings::ToolbarZoom11(), this);
  m_zoom11Action->setShortcut(QKeySequence("Ctrl+1"));
  m_zoom11Action->setToolTip(UiStrings::ToolbarZoom11Tooltip());
  connect(m_zoom11Action, &QAction::triggered, this, &RibbonBar::Zoom11Requested);

  m_fitContentAction = new QAction(UiIcons::FitContent(), UiStrings::ToolbarFitContent(), this);
  m_fitContentAction->setShortcut(QKeySequence("Ctrl+F"));
  m_fitContentAction->setToolTip(UiStrings::ToolbarFitContentTooltip());
  connect(m_fitContentAction, &QAction::triggered, this, &RibbonBar::FitContentRequested);

  QToolButton* fitBtn = new QToolButton(this);
  fitBtn->setDefaultAction(m_fitAction);
  fitBtn->setIconSize(QSize(24, 24));
  fitBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  fitBtn->setMinimumSize(60, 56);
  gl->addWidget(fitBtn);

  QToolButton* zoomBtn = new QToolButton(this);
  zoomBtn->setDefaultAction(m_zoom11Action);
  zoomBtn->setIconSize(QSize(24, 24));
  zoomBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  zoomBtn->setMinimumSize(60, 56);
  gl->addWidget(zoomBtn);

  QToolButton* fitContentBtn = new QToolButton(this);
  fitContentBtn->setDefaultAction(m_fitContentAction);
  fitContentBtn->setIconSize(QSize(24, 24));
  fitContentBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  fitContentBtn->setMinimumSize(60, 56);
  gl->addWidget(fitContentBtn);

  return group;
}

QWidget* RibbonBar::BuildMeasureGroup() {
  RibbonGroup* group = new RibbonGroup(UiStrings::RibbonMeasureGroup());
  QHBoxLayout* gl = new QHBoxLayout(group);
  gl->setSpacing(4);

  m_measureAction = new QAction(UiIcons::Measure(), UiStrings::ToolbarMeasure(), this);
  m_measureAction->setCheckable(true);
  connect(m_measureAction, &QAction::toggled, this, &RibbonBar::MeasureToggled);

  m_angleAction = new QAction(UiIcons::Angle(), UiStrings::ToolbarAngle(), this);
  m_angleAction->setCheckable(true);
  connect(m_angleAction, &QAction::toggled, this, &RibbonBar::AngleToggled);

  m_circleRoiAction = new QAction(UiIcons::CircleRoi(), UiStrings::ToolbarCircleRoi(), this);
  m_circleRoiAction->setCheckable(true);
  connect(m_circleRoiAction, &QAction::toggled, this, &RibbonBar::CircleRoiToggled);

  m_rectRoiAction = new QAction(UiIcons::RectRoi(), UiStrings::ToolbarRectRoi(), this);
  m_rectRoiAction->setCheckable(true);
  connect(m_rectRoiAction, &QAction::toggled, this, &RibbonBar::RectRoiToggled);

  QToolButton* measureBtn = new QToolButton(this);
  measureBtn->setDefaultAction(m_measureAction);
  measureBtn->setIconSize(QSize(24, 24));
  measureBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  measureBtn->setMinimumSize(60, 56);
  gl->addWidget(measureBtn);

  QToolButton* angleBtn = new QToolButton(this);
  angleBtn->setDefaultAction(m_angleAction);
  angleBtn->setIconSize(QSize(24, 24));
  angleBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  angleBtn->setMinimumSize(60, 56);
  gl->addWidget(angleBtn);

  QToolButton* circleBtn = new QToolButton(this);
  circleBtn->setDefaultAction(m_circleRoiAction);
  circleBtn->setIconSize(QSize(24, 24));
  circleBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  circleBtn->setMinimumSize(60, 56);
  gl->addWidget(circleBtn);

  QToolButton* rectBtn = new QToolButton(this);
  rectBtn->setDefaultAction(m_rectRoiAction);
  rectBtn->setIconSize(QSize(24, 24));
  rectBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  rectBtn->setMinimumSize(60, 56);
  gl->addWidget(rectBtn);

  return group;
}

QWidget* RibbonBar::BuildClearGroup() {
  RibbonGroup* group = new RibbonGroup(UiStrings::RibbonClearGroup());
  QHBoxLayout* gl = new QHBoxLayout(group);
  gl->setSpacing(4);

  m_clearMeasuresAction = new QAction(UiIcons::Clear(), UiStrings::ToolbarClearMeasures(), this);
  connect(m_clearMeasuresAction, &QAction::triggered, this, &RibbonBar::ClearMeasurementsRequested);

  QToolButton* clearBtn = new QToolButton(this);
  clearBtn->setDefaultAction(m_clearMeasuresAction);
  clearBtn->setIconSize(QSize(24, 24));
  clearBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  clearBtn->setMinimumSize(60, 56);
  gl->addWidget(clearBtn);

  return group;
}

QWidget* RibbonBar::BuildRotateGroup() {
  RibbonGroup* group = new RibbonGroup(UiStrings::RibbonRotateGroup());
  QHBoxLayout* gl = new QHBoxLayout(group);
  gl->setSpacing(4);

  m_rotateCwAction = new QAction(UiIcons::RotateCw(), UiStrings::ToolbarRotateCw(), this);
  m_rotateCwAction->setShortcut(QKeySequence("R"));
  m_rotateCwAction->setToolTip(UiStrings::ToolbarRotateCwTooltip());
  connect(m_rotateCwAction, &QAction::triggered, this, &RibbonBar::RotateCwRequested);

  m_rotateCcwAction = new QAction(UiIcons::RotateCcw(), UiStrings::ToolbarRotateCcw(), this);
  m_rotateCcwAction->setShortcut(QKeySequence("Shift+R"));
  m_rotateCcwAction->setToolTip(UiStrings::ToolbarRotateCcwTooltip());
  connect(m_rotateCcwAction, &QAction::triggered, this, &RibbonBar::RotateCcwRequested);

  QToolButton* cwBtn = new QToolButton(this);
  cwBtn->setDefaultAction(m_rotateCwAction);
  cwBtn->setIconSize(QSize(24, 24));
  cwBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  cwBtn->setMinimumSize(60, 56);
  gl->addWidget(cwBtn);

  QToolButton* ccwBtn = new QToolButton(this);
  ccwBtn->setDefaultAction(m_rotateCcwAction);
  ccwBtn->setIconSize(QSize(24, 24));
  ccwBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  ccwBtn->setMinimumSize(60, 56);
  gl->addWidget(ccwBtn);

  return group;
}

QWidget* RibbonBar::BuildFlipGroup() {
  RibbonGroup* group = new RibbonGroup(UiStrings::RibbonFlipGroup());
  QHBoxLayout* gl = new QHBoxLayout(group);
  gl->setSpacing(4);

  m_flipHAction = new QAction(UiIcons::FlipH(), UiStrings::ToolbarFlipH(), this);
  m_flipHAction->setShortcut(QKeySequence("H"));
  m_flipHAction->setToolTip(UiStrings::ToolbarFlipHTooltip());
  connect(m_flipHAction, &QAction::triggered, this, &RibbonBar::FlipHRequested);

  m_flipVAction = new QAction(UiIcons::FlipV(), UiStrings::ToolbarFlipV(), this);
  m_flipVAction->setShortcut(QKeySequence("V"));
  m_flipVAction->setToolTip(UiStrings::ToolbarFlipVTooltip());
  connect(m_flipVAction, &QAction::triggered, this, &RibbonBar::FlipVRequested);

  QToolButton* hBtn = new QToolButton(this);
  hBtn->setDefaultAction(m_flipHAction);
  hBtn->setIconSize(QSize(24, 24));
  hBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  hBtn->setMinimumSize(60, 56);
  gl->addWidget(hBtn);

  QToolButton* vBtn = new QToolButton(this);
  vBtn->setDefaultAction(m_flipVAction);
  vBtn->setIconSize(QSize(24, 24));
  vBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  vBtn->setMinimumSize(60, 56);
  gl->addWidget(vBtn);

  return group;
}

QWidget* RibbonBar::BuildWlGroup() {
  RibbonGroup* group = new RibbonGroup(UiStrings::RibbonWlGroup());
  QHBoxLayout* gl = new QHBoxLayout(group);
  gl->setSpacing(4);

  QAction* wlAction = new QAction(UiIcons::Wl(), UiStrings::ToolbarAdjustWl(), this);
  wlAction->setCheckable(true);
  wlAction->setShortcut(QKeySequence("W"));
  wlAction->setToolTip(UiStrings::ToolbarAdjustWlTooltip());
  connect(wlAction, &QAction::toggled, this, &RibbonBar::WindowLevelToggled);

  QToolButton* wlBtn = new QToolButton(this);
  wlBtn->setDefaultAction(wlAction);
  wlBtn->setIconSize(QSize(24, 24));
  wlBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  wlBtn->setMinimumSize(60, 56);
  gl->addWidget(wlBtn);

  return group;
}

QWidget* RibbonBar::BuildPresetGroup() {
  RibbonGroup* group = new RibbonGroup(UiStrings::RibbonPresetGroup());
  QHBoxLayout* gl = new QHBoxLayout(group);
  gl->setSpacing(4);

  QLabel* presetLabel = new QLabel(UiStrings::ToolbarPresetLabel(), this);
  QPalette labelPal = presetLabel->palette();
  labelPal.setColor(QPalette::WindowText, RibbonStyles::DimText());
  presetLabel->setPalette(labelPal);
  QFont labelFont;
  labelFont.setPointSize(11);
  presetLabel->setFont(labelFont);
  gl->addWidget(presetLabel);

  m_presetCombo = new QComboBox(this);
  m_presetCombo->setMinimumWidth(260);
  m_presetCombo->setMaximumWidth(350);
  m_presetCombo->setEnabled(false);
  m_presetCombo->setMinimumHeight(24);
  m_presetCombo->setContentsMargins(4, 4, 8, 4);
  QPalette comboPal = m_presetCombo->palette();
  comboPal.setColor(QPalette::Window, RibbonStyles::ButtonBg());
  comboPal.setColor(QPalette::WindowText, RibbonStyles::TextColor());
  comboPal.setColor(QPalette::Base, RibbonStyles::ButtonBg());
  comboPal.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
  comboPal.setColor(QPalette::Highlight, RibbonStyles::ButtonChecked());
  m_presetCombo->setPalette(comboPal);
  connect(m_presetCombo, &QComboBox::currentIndexChanged,
          this, &RibbonBar::WindowPresetChanged);
  gl->addWidget(m_presetCombo);

  return group;
}

void RibbonBar::SetMeasurementActionsEnabled(bool enabled) {
  if (m_measureAction) m_measureAction->setEnabled(enabled);
  if (m_angleAction) m_angleAction->setEnabled(enabled);
  if (m_circleRoiAction) m_circleRoiAction->setEnabled(enabled);
  if (m_rectRoiAction) m_rectRoiAction->setEnabled(enabled);
  if (m_clearMeasuresAction) m_clearMeasuresAction->setEnabled(enabled);
}

void RibbonBar::SetNavigationActionsEnabled(bool enabled) {
  if (m_fitAction) m_fitAction->setEnabled(enabled);
  if (m_zoom11Action) m_zoom11Action->setEnabled(enabled);
  if (m_fitContentAction) m_fitContentAction->setEnabled(enabled);
  if (m_rotateCwAction) m_rotateCwAction->setEnabled(enabled);
  if (m_rotateCcwAction) m_rotateCcwAction->setEnabled(enabled);
  if (m_flipHAction) m_flipHAction->setEnabled(enabled);
  if (m_flipVAction) m_flipVAction->setEnabled(enabled);
}

void RibbonBar::PopulateWindowPresets(const QStringList& items, int activeIndex, bool enable) {
  if (!m_presetCombo) return;

  m_presetCombo->blockSignals(true);
  m_presetCombo->clear();
  m_presetCombo->addItems(items);
  if (activeIndex >= 0 && activeIndex < m_presetCombo->count()) {
    m_presetCombo->setCurrentIndex(activeIndex);
  }
  m_presetCombo->setEnabled(enable);
  m_presetCombo->blockSignals(false);
}

void RibbonBar::SelectWindowPreset(int index) {
  if (m_presetCombo) {
    m_presetCombo->blockSignals(true);
    m_presetCombo->setCurrentIndex(index);
    m_presetCombo->blockSignals(false);
  }
}

void RibbonBar::UpdateCustomPresetText(const QString& text) {
  if (!m_presetCombo || m_presetCombo->count() == 0) return;
  const int lastIndex = m_presetCombo->count() - 1;
  m_presetCombo->setItemText(lastIndex, text);
}

}  // namespace presentation

#include "RibbonBar.moc"
