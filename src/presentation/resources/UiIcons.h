#pragma once

#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace UiIcons {

inline QIcon MakeIcon(const QSize& size, const QColor& color, QPainterPath&& path) {
  QPixmap pixmap(size);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(Qt::NoPen);
  painter.setBrush(color);
  painter.drawPath(std::move(path));
  return QIcon(std::move(pixmap));
}

inline QIcon MakeStrokeIcon(const QSize& size, const QColor& color, double width,
                            QPainterPath&& path) {
  QPixmap pixmap(size);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter.setBrush(Qt::NoBrush);
  painter.drawPath(std::move(path));
  return QIcon(std::move(pixmap));
}

inline QColor Accent() { return QColor(0x56, 0x9c, 0xd6); }
inline QColor Light()  { return QColor(0xcc, 0xcc, 0xcc); }
inline QColor Gold()   { return QColor(0xff, 0xd7, 0x00); }
inline QColor Green()  { return QColor(0x00, 0xff, 0x96); }
inline QColor Blue()   { return QColor(0x64, 0x96, 0xff); }
inline QColor Pink()   { return QColor(0xff, 0x64, 0xc8); }

inline QIcon Open() {
  QPainterPath path;
  QPainterPath tab;
  tab.addRect(4, 1, 8, 3);
  path.addRect(1, 3, 14, 10);
  path -= tab;
  return MakeIcon(QSize(16, 16), Accent(), std::move(path));
}

inline QIcon Close() {
  QPainterPath path;
  path.moveTo(3, 3); path.lineTo(13, 13);
  path.moveTo(13, 3); path.lineTo(3, 13);
  return MakeStrokeIcon(QSize(16, 16), QColor(0xff, 0x6b, 0x6b), 2.0, std::move(path));
}

inline QIcon Fit() {
  QPainterPath path;
  path.moveTo(2, 6); path.lineTo(2, 2); path.lineTo(6, 2);
  path.moveTo(14, 6); path.lineTo(14, 2); path.lineTo(10, 2);
  path.moveTo(2, 10); path.lineTo(2, 14); path.lineTo(6, 14);
  path.moveTo(14, 10); path.lineTo(14, 14); path.lineTo(10, 14);
  return MakeStrokeIcon(QSize(16, 16), Light(), 1.5, std::move(path));
}

inline QIcon Zoom11() {
  QPainterPath path;
  path.addEllipse(1.5, 1.5, 10, 10);
  path.lineTo(14, 14);
  path.moveTo(4, 6.5); path.lineTo(10, 6.5);
  path.moveTo(7, 3.5); path.lineTo(7, 9.5);
  return MakeStrokeIcon(QSize(16, 16), Light(), 1.2, std::move(path));
}

inline QIcon FitContent() {
  QPainterPath path;
  path.addRect(2, 2, 12, 12);
  path.moveTo(5, 5); path.lineTo(11, 5); path.lineTo(11, 11); path.lineTo(5, 11); path.closeSubpath();
  return MakeStrokeIcon(QSize(16, 16), Accent(), 1.2, std::move(path));
}

inline QIcon Measure() {
  QPainterPath path;
  path.moveTo(2, 14); path.lineTo(14, 2);
  path.moveTo(5, 11); path.lineTo(4, 10);
  path.moveTo(8, 8);  path.lineTo(7, 7);
  path.moveTo(11, 5); path.lineTo(10, 4);
  path.addEllipse(2, 14, 2, 2);
  path.addEllipse(14, 2, 2, 2);
  return MakeStrokeIcon(QSize(16, 16), Gold(), 1.5, std::move(path));
}

inline QIcon Angle() {
  QPainterPath path;
  path.moveTo(2, 14); path.lineTo(8, 2); path.lineTo(14, 14);
  path.moveTo(5, 10); path.arcTo(4, 7, 8, 6, 0, -60);
  return MakeStrokeIcon(QSize(16, 16), Green(), 1.5, std::move(path));
}

inline QIcon CircleRoi() {
  QPainterPath path;
  path.addEllipse(2, 2, 12, 12);
  path.addEllipse(7, 7, 2, 2);
  return MakeStrokeIcon(QSize(16, 16), Blue(), 1.5, std::move(path));
}

inline QIcon RectRoi() {
  QPainterPath path;
  path.addRect(2, 2, 12, 12);
  return MakeStrokeIcon(QSize(16, 16), Pink(), 1.5, std::move(path));
}

inline QIcon Clear() {
  QPainterPath path;
  path.moveTo(4, 4); path.lineTo(12, 12);
  path.moveTo(12, 4); path.lineTo(4, 12);
  return MakeStrokeIcon(QSize(16, 16), Light(), 1.5, std::move(path));
}

inline QIcon RotateCw() {
  QPainterPath path;
  path.moveTo(3, 8);
  path.arcTo(2, 2, 12, 12, 45, -270);
  path.lineTo(14, 5); path.lineTo(14, 2);
  return MakeStrokeIcon(QSize(16, 16), Light(), 1.5, std::move(path));
}

inline QIcon RotateCcw() {
  QPainterPath path;
  path.moveTo(13, 8);
  path.arcTo(2, 2, 12, 12, 225, -270);
  path.lineTo(2, 11); path.lineTo(2, 14);
  return MakeStrokeIcon(QSize(16, 16), Light(), 1.5, std::move(path));
}

inline QIcon FlipH() {
  QPainterPath path;
  path.moveTo(8, 2); path.lineTo(8, 14);
  path.moveTo(3, 5); path.lineTo(1, 8); path.lineTo(3, 11);
  path.moveTo(13, 5); path.lineTo(15, 8); path.lineTo(13, 11);
  return MakeStrokeIcon(QSize(16, 16), Light(), 1.2, std::move(path));
}

inline QIcon FlipV() {
  QPainterPath path;
  path.moveTo(2, 8); path.lineTo(14, 8);
  path.moveTo(5, 3); path.lineTo(8, 1); path.lineTo(11, 3);
  path.moveTo(5, 13); path.lineTo(8, 15); path.lineTo(11, 13);
  return MakeStrokeIcon(QSize(16, 16), Light(), 1.2, std::move(path));
}

inline QIcon Wl() {
  QPainterPath path;
  path.addRect(1, 3, 14, 10);
  path.moveTo(5, 13); path.lineTo(5, 15);
  path.moveTo(11, 13); path.lineTo(11, 15);
  return MakeStrokeIcon(QSize(16, 16), Light(), 1.2, std::move(path));
}

}  // namespace UiIcons
