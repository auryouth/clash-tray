#pragma once

#include <qcontainerfwd.h>

#include <tuple>

class Utils {
 public:
  static auto dirExists(const QString& dirPath, QString& message) -> bool;
  static auto fileExists(const QString& filePath, QString& message) -> bool;
  static void openUrl(const QString& url);
  static void openDir(const QString& dirPath);
  static void openFile(const QString& filePath);
  static auto regexMatch(const QString& regexPattern,
                         const QString& text) -> std::tuple<bool, QString>;
  static auto commandFind(const QString& command) -> std::tuple<bool, QString>;
  static auto commandFind(const QStringList& commands)
      -> std::tuple<bool, QString>;
  static auto commandCheck(const QString& command) -> std::tuple<bool, QString>;
};
