#include "MainWindow.h"
#include "./ui_MainWindow.h"

#include <QKeyEvent>
#include <QMatrix4x4>
#include <QPainter>
#include <algorithm>

constexpr QRgb white   = qRgba(255, 255, 255, 255);
constexpr QRgb green   = qRgba(  0, 255, 0, 255);
constexpr QRgb red     = qRgba(255,   0,   0, 255);
constexpr QRgb blue    = qRgba( 64, 128, 255, 255);
constexpr QRgb purple  = qRgba(128, 128, 255, 255);
constexpr QRgb orange  = qRgba(0xff, 0x5c, 0, 255);
constexpr QRgb magenta = qRgba(255,   0, 255, 255);
constexpr QRgb cyan    = qRgba(  0, 255, 255, 255);
constexpr QRgb yellow  = qRgba(255, 200,   0, 255);

MainWindow::MainWindow (int width, int height, QWidget *parent)
    : QMainWindow (parent), ui (new Ui::MainWindow), /*fb(64, 64)*/  fb(width, height)
{
  setFixedSize(width, height);
  //setWindowFlags(Qt::FramelessWindowHint);
  ui->setupUi (this);
  setStatusBar(nullptr);

  QStringList args = QCoreApplication::arguments();
  if (args.size() > 1)
    {
      const QString filename = args.at(1);
      qDebug() << QString("Reading OBJ file %1 from the argument list").arg(filename);
      model = Model::readObjFile(filename);
    }
}

MainWindow::~MainWindow () { delete ui; }

void
MainWindow::paintEvent (QPaintEvent *event)
{
  fb.clear(QColor(0,0,0,0));

  QElapsedTimer timer;
  timer.start();
  if (model.has_value())
    {
      drawModel();
    }
  else
    {
      drawShapes();
    }
  qint64 elapsedMs = timer.elapsed();
  qDebug() << "Frame drawn in " << elapsedMs << "ms";

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, false);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
  painter.fillRect(rect(), Qt::black);
  painter.drawImage(rect(), fb.qimage());


}

void
MainWindow::drawShapes ()
{
  int ax =  7, ay =  3;
  int bx = 12, by = 37;
  int cx = 62, cy = 53;

  if (drawTriangle)
    {
      fb.triangle({ax, ay}, {bx, by}, {cx, cy}, blue);
    }
  else if (drawTriangle2)
    {
      fb.triangle2({ax, ay}, {bx, by}, {cx, cy}, orange);
    }
  else if (drawTriangle3)
    {
      fb.triangle3({ax, ay}, {cx, cy}, {bx, by}, magenta);
    }
  else if (drawTriangle4)
    {
      fb.triangle4({ax, ay}, {50, 20}, {cx, cy}, magenta);
      fb.triangle4({ax, ay}, {cx, cy}, {bx, by}, cyan);
    }
  else if (drawTriangle5)
    {
      fb.triangle5({ax, ay}, {50, 20}, {cx, cy}, magenta);
      fb.triangle5({ax, ay}, {cx, cy}, {bx, by}, cyan);
    }
  else if (drawTriangle6)
    {
      fb.triangle6({ax, ay}, {50, 20}, {cx, cy}, magenta);
      fb.triangle6({ax, ay}, {cx, cy}, {bx, by}, cyan);
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
}

inline void
MainWindow::drawWireframeTriangle (const QVector3D &v0, const QVector3D &v1,
                                   const QVector3D &v2, QColor c)
{
  // To draw a triangle in wireframe mode, draw its three edges.
  // The model is supposed to fit in the [-1,1]^3 cube, so we translate it to [0,2]^3
  // and then scale it to (0, width/2) in the X axis and (0, height/2) in the Y axis.

  float halfWidth = fb.width()/2.0f;
  float halfHeight = fb.height()/2.0f;

  int x0 = (v0.x() + 1)*halfWidth;
  int x1 = (v1.x() + 1)*halfWidth;
  int x2 = (v2.x() + 1)*halfWidth;
  int y0 = (v0.y() + 1)*halfHeight;
  int y1 = (v1.y() + 1)*halfHeight;
  int y2 = (v2.y() + 1)*halfHeight;

  fb.line(x0, y0, x1, y1, c);
  fb.line(x1, y1, x2, y2, c);
  fb.line(x2, y2, x1, y1, c);
}

inline point
MainWindow::project(const QVector3D &v)
{
  int x = std::clamp((int)std::round((v.x()+1)*fb.width()/2.0), 0, fb.width()-1);
  int y = std::clamp((int)std::round((v.y()+1)*fb.height()/2.0), 0, fb.height()-1);
  return {x, y};
}

void
MainWindow::drawModel ()
{
  const QVector<QVector3D> &vertices = model->vertices();
  const QVector<uint16_t> &indices = model->indices();


  void (FrameBuffer::*triangleFunc)(point p, point q, point r, QColor c) = nullptr;
  if      (drawTriangle)  triangleFunc = &FrameBuffer::triangle;
  else if (drawTriangle2) triangleFunc = &FrameBuffer::triangle2;
  else if (drawTriangle3) triangleFunc = &FrameBuffer::triangle3;
  else if (drawTriangle4) triangleFunc = &FrameBuffer::triangle4;
  else if (drawTriangle5) triangleFunc = &FrameBuffer::triangle5;
  else if (drawTriangle6) triangleFunc = &FrameBuffer::triangle6;


  std::srand(0x1u);
  QQuaternion q = QQuaternion::fromAxisAndAngle(QVector3D(0,1,0), yRot);

  // A Model read by Model::readObjFile() is guaranteed to have a number indices that is a multiple
  // of 3.
  for (int i = 0; i < indices.size()/3; i++)
    {
      const QVector3D &v0 = q.rotatedVector(vertices[indices[3*i+0]]);
      const QVector3D &v1 = q.rotatedVector(vertices[indices[3*i+1]]);
      const QVector3D &v2 = q.rotatedVector(vertices[indices[3*i+2]]);

      //drawWireframeTriangle(v0, v1, v2, c);
      QColor c(std::rand()%255, std::rand()%255, std::rand()%255, 255);
      if (triangleFunc != nullptr)
        {
          (fb.*triangleFunc)(project(v0), project(v1), project(v2), c);
        }
    }

  // int w = fb.width();
  // int h = fb.height();
  // for (int i = 0; i < vertices.size(); i++)
  //   {
  //     const QVector3D &v = vertices[i];
  //     int x = std::clamp((int)std::round((v.x()+1)*w/2.0), 0, w-1);
  //     int y = std::clamp((int)std::round((v.y()+1)*h/2.0), 0, h-1);
  //     fb.set(x, y, white);
  //   }
}

void
MainWindow::keyPressEvent (QKeyEvent *e)
{
  bool stateChange = false;

  int key = e->key();
  if (Qt::Key_1 <= key && key <= Qt::Key_8)
    {
      stateChange = true;
    }
  if (Qt::Key_1 <= key && key <= Qt::Key_6)
    {
      drawTriangle = false;
      drawTriangle2 = false;
      drawTriangle3 = false;
      drawTriangle4 = false;
      drawTriangle5 = false;
      drawTriangle6 = false;
    }

  if (e->key() == Qt::Key_1)
    {
      drawTriangle2 = true; // drawTriangle is under maintenance :p
    }
  else if (e->key() == Qt::Key_2)
    {
      drawTriangle2 = true;
    }
  else if (e->key() == Qt::Key_3)
    {
      drawTriangle3 = true;
    }
  else if (e->key() == Qt::Key_4)
    {
      drawTriangle4 = true;
    }
  else if (e->key() == Qt::Key_5)
    {
      drawTriangle5 = true;
    }
  else if (e->key() == Qt::Key_6)
    {
      drawTriangle6 = true;
    }
  else if (e->key() == Qt::Key_7)
    {
      drawLines = !drawLines;
    }
  else if (e->key() == Qt::Key_8)
    {
      drawPoints = !drawPoints;
    }
  else if (e->key() == Qt::Key_Escape)
    {
      QApplication::exit(0);
    }

  if (key == Qt::Key_Right)
    {
      yRot += 36;
      yRot %= 360;
      stateChange = true;
    }
  else if (key == Qt::Key_Left)
    {
      yRot -= 36;
      yRot %= 360;
      stateChange = true;
    }

  if (stateChange)
    {
      update();
    }
}
