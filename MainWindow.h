#pragma once

#include "FrameBuffer.h"

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
  MainWindow (QWidget *parent = nullptr);
  ~MainWindow ();

protected:
  void paintEvent(QPaintEvent *event) override;
  void keyPressEvent(QKeyEvent *e) override;

private:
  Ui::MainWindow *ui;
  FrameBuffer fb;
  QLabel bg;

  bool drawTriangle = false;
  bool drawTriangle2 = false;
  bool drawTriangle3 = true;
  bool drawTriangle4 = true;
  bool drawLines = false;
  bool drawPoints = false;
};
