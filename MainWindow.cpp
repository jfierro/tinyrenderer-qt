#include "MainWindow.h"
#include "./ui_MainWindow.h"

#include <QKeyEvent>
#include <QPainter>

constexpr QRgb white   = qRgba(255, 255, 255, 255);
constexpr QRgb green   = qRgba(  0, 255, 0, 255);
constexpr QRgb red     = qRgba(255,   0,   0, 255);
constexpr QRgb blue    = qRgba( 64, 128, 255, 255);
constexpr QRgb purple  = qRgba(128, 128, 255, 255);
constexpr QRgb orange  = qRgba(0xff, 0x5c, 0, 255);
constexpr QRgb magenta = qRgba(255,   0, 255, 255);
constexpr QRgb cyan    = qRgba(  0, 255, 255, 255);
constexpr QRgb yellow  = qRgba(255, 200,   0, 255);

MainWindow::MainWindow (QWidget *parent)
    : QMainWindow (parent), ui (new Ui::MainWindow), fb(64, 64)
{
  ui->setupUi (this);
  setStatusBar(nullptr);
}

MainWindow::~MainWindow () { delete ui; }

void
MainWindow::paintEvent (QPaintEvent *event)
{
  int ax =  7, ay =  3;
  int bx = 12, by = 37;
  int cx = 62, cy = 53;

  fb.clear(Qt::black);

  if (drawTriangle)
    {
      fb.triangle({ax, ay}, {bx, by}, {cx, cy}, blue);
    }

  if (drawTriangle2)
    {
      fb.triangle2({ax, ay}, {bx, by}, {cx, cy}, orange);
    }

  if (drawTriangle3)
    {
      fb.triangle3({ax, ay}, {bx, by}, {cx, cy}, magenta);
    }

  if (drawTriangle4)
    {
      fb.triangle4({ax, ay}, {bx, by}, {cx, cy}, cyan);
    }

  if (drawLines)
    {
      fb.line(ax, ay, bx, by, purple);
      fb.line(cx, cy, bx, by, green);
      fb.line(cx, cy, ax, ay, yellow);
      fb.line(ax, ay, cx, cy, red);
    }

  if (drawPoints)
    {
      fb.set(ax, ay, white);
      fb.set(bx, by, white);
      fb.set(cx, cy, white);
    }

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, false);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
  painter.fillRect(rect(), Qt::black);
  painter.drawImage(rect(), fb.qimage());
}

void
MainWindow::keyPressEvent (QKeyEvent *e)
{
  bool stateChange = false;
  if (e->key() == Qt::Key_1)
    {
      drawTriangle = !drawTriangle;
      stateChange = true;
    }
  else if (e->key() == Qt::Key_2)
    {
      drawTriangle2 = !drawTriangle2;
      stateChange = true;
    }
  else if (e->key() == Qt::Key_3)
    {
      drawTriangle3 = !drawTriangle3;
      stateChange = true;
    }
  else if (e->key() == Qt::Key_4)
    {
      drawTriangle4 = !drawTriangle4;
      stateChange = true;
    }
  else if (e->key() == Qt::Key_5)
    {
      drawLines = !drawLines;
      stateChange = true;
    }
  else if (e->key() == Qt::Key_6)
    {
      drawPoints = !drawPoints;
      stateChange = true;
    }
  if (stateChange)
    {
      update();
    }
}
