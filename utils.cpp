#include "utils.hpp"

#include <qlogging.h>

#include <QDesktopServices>
#include <QDir>
#include <QUrl>

auto Utils::dirExists(const QString& dirPath, QString& message) -> bool {
  if (dirPath.isEmpty()) {
    message = QString("dirPath is empty: ") + dirPath;

    return false;
  }
  if (!QDir(dirPath).exists()) {
    message = QString("dirPath does not exist: ") + dirPath;

    return false;
  }

  message = QString("Found dirPath: ") + dirPath;
  return true;
}

void Utils::openUrl(const QString& url) {
  qInfo() << __func__ << url;
  QDesktopServices::openUrl(QUrl(url));
}

void Utils::openDir(const QString& dirPath) {
  qInfo() << __func__ << dirPath;
  QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath));
}
