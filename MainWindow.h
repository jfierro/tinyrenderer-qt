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

private:
  Ui::MainWindow *ui;
  FrameBuffer fb;
  QLabel bg;
  std::optional<Model> model;

  bool drawTriangle = false;
  bool drawTriangle2 = false;
  bool drawTriangle3 = true;
  bool drawTriangle4 = true;
  bool drawLines = false;
  bool drawPoints = false;
};
