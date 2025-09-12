#include "MainWindow.h"

#include <QApplication>

int
main (int argc, char *argv[])
{
  QApplication a (argc, argv);
  MainWindow w;
  w.setFixedSize(256,256);
  w.show ();
  return a.exec ();
}
