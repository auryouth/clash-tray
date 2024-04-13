#include <QApplication>

#include "clashtray.hpp"
#include "logger.hpp"

auto main(int argc, char* argv[]) -> int {
  Logger::init();

  QApplication app(argc, argv);
  QApplication::setQuitOnLastWindowClosed(false);
  ClashTray clashTray;
  int state = app.exec();

  Logger::clean();
  return state;
}
