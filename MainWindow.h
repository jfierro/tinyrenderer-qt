#pragma once

#include "FrameBuffer.h"
#include "Model.h"

#include <QMainWindow>
#include <qlabel.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow (int width, int height, QWidget *parent = nullptr);
  ~MainWindow ();

protected:
  void paintEvent(QPaintEvent *event) override;
  void keyPressEvent(QKeyEvent *e) override;
  void drawShapes();
  void drawModel();
  void drawWireframeTriangle (const QVector3D &v0, const QVector3D &v1,
                              const QVector3D &v2, QColor c);
  point project(const QVector3D &v);
  point3 project3(const QVector3D &v);

private:
  int w, h;
  Ui::MainWindow *ui;
  FrameBuffer fb;
  QLabel bg;
  std::optional<Model> model;
  int yRot = 0;

  bool drawTriangle = false;
  bool drawTriangle2 = true;
  bool drawTriangle3 = false;
  bool drawTriangle4 = false;
  bool drawTriangle5 = false;
  bool drawTriangle6 = false;
  bool drawLines = false;
  bool drawPoints = false;

  bool depthTesting{false};
};
