#include "MainWindow.h"

#include <QApplication>

int
main (int argc, char *argv[])
{
  QApplication a (argc, argv);
  MainWindow w(800, 800);
  w.show ();
  return a.exec ();
}
