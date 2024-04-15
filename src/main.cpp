#include <QApplication>

#include "clashtray.hpp"
#include "utils/logger.hpp"

auto main(int argc, char* argv[]) -> int {
  QApplication app(argc, argv);
  QApplication::setQuitOnLastWindowClosed(false);
  Logger::initLog();

  ClashTray clashTray;
  return app.exec();
}
