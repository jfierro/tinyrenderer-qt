#pragma once

#include <QPainter>
//#include <QPixmap>
#include <QImage>

struct point
{
  int x;
  int y;
};

class FrameBuffer
{
public:
  FrameBuffer(int w, int h);

  void clear(QColor c);
  const QImage &qimage() const;

  void set(int x, int y, QColor c);
  void line(int ax, int ay, int bx, int by, QColor c);
  void triangle(point p, point q, point r, QColor c);
  void triangle2(point p, point q, point r, QColor c);
  void triangle3(point p, point q, point r, QColor c);
  void triangle4(point p, point q, point r, QColor c);
  void scanline(int y, int xleft, int xright, QColor c);

private:
  QImage frameBuffer;
};
