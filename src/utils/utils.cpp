#include "utils/utils.hpp"

#include <qcontainerfwd.h>

#include <QDesktopServices>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QUrl>
#include <tuple>

#include "utils/logger.hpp"

auto Utils::dirExists(const QString& dirPath, QString& message) -> bool {
  if (!QDir(dirPath).exists()) {
    LOG_WARNING << "dirPath does not exist:" << dirPath;
    message = "dirPath does not exist: " + dirPath;

    return false;
  }

  LOG_INFO << "Found dirPath:" << dirPath;
  message = "Found dirPath: " + dirPath;
  return true;
}

void Utils::openUrl(const QString& url) {
  LOG_INFO << __FUNCTION__ << url;
  QDesktopServices::openUrl(QUrl(url));
}

void Utils::openDir(const QString& dirPath) {
  LOG_INFO << __FUNCTION__ << dirPath;
  QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath));
}

auto Utils::regexMatch(const QString& regexPattern,
                       const QString& text) -> std::tuple<bool, QString> {
  LOG_INFO.nospace() << __FUNCTION__ << ": " << regexPattern << " in " << text;

  QRegularExpression regex(regexPattern);

  LOG_INFO << "check regex validity";
  if (!regex.isValid()) {
    LOG_CRITICAL << "Regex invalid:" << regex.errorString();

    return std::make_tuple(false, regex.errorString());
  }
  LOG_INFO << "Regex valid";

  QRegularExpressionMatch match = regex.match(text);

  if (match.hasMatch()) {
    LOG_INFO << "Match found:" << match.captured(0);

    return std::make_tuple(true, match.captured(0));
  }
  LOG_WARNING << "Match not found";

  return std::make_tuple(false, "Match not found");
}

auto Utils::commandFind(const QString& command) -> std::tuple<bool, QString> {
  LOG_INFO << __FUNCTION__ << command;

  QProcess pfind;
  pfind.start("where", QStringList() << command);  // Windows only

  if (pfind.waitForFinished() && pfind.exitCode() == 0) {
    LOG_INFO << "Command successfully found";

    return std::make_tuple(true, command);
  }
  QString err = pfind.errorString();
  LOG_WARNING << "Command not found:" << err;

  return std::make_tuple(false, err);
}

auto Utils::commandFind(const QStringList& commands)
    -> std::tuple<bool, QString> {
  LOG_INFO << __FUNCTION__ << commands;

  for (const auto& command : commands) {
    auto [success, result] = commandFind(command);
    if (success) {
      LOG_INFO << "Command found in list:" << result;

      return std::make_tuple(true, result);
    }

    LOG_WARNING << "Command not found in list:" << result;
  }

  return std::make_tuple(false,
                         "Command not found in list" + commands.join(","));
}

auto Utils::commandCheck(const QString& command) -> std::tuple<bool, QString> {
  LOG_INFO << __FUNCTION__ << command;

  QProcess pcheck;
  pcheck.startCommand(command);

  if (pcheck.waitForFinished() && pcheck.exitCode() == 0) {
    LOG_INFO << "Command check success";
    return std::make_tuple(true, pcheck.readAllStandardOutput());
  }

  QString err = pcheck.errorString();
  LOG_WARNING << "Command check failed:" << err;

  return std::make_tuple(false, err);
}
