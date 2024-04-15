#pragma once

#include <QString>

class Utils {
 public:
  static auto dirExists(const QString& dirPath, QString& message) -> bool;
  static void openUrl(const QString& url);
  static void openDir(const QString& dirPath);
};
