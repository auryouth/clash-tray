#include "utils/logger.hpp"

#include <qcontainerfwd.h>
#include <qlogging.h>
#include <qstringconverter_base.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QMutex>
#include <QStringLiteral>
#include <QTextStream>

namespace Logger {

static QDateTime startUpTime;
static QString gLogDir;
static QString messagePattern;
static QFile file;

static void outputMessage(QtMsgType type, const QMessageLogContext& context,
                          const QString& msg);

static void setupPattern() {
  QString colorCodePattern = QStringLiteral(
      "%{if-debug}\x1b[37m%{endif}"      // White
      "%{if-info}\x1b[32m%{endif}"       // Green
      "%{if-warning}\x1b[35m%{endif}"    // Magenta
      "%{if-critical}\x1b[31m%{endif}"   // Red
      "%{if-fatal}\x1b[31;1m%{endif}");  // Red and bold

  QString resetCode = QStringLiteral("\x1b[0m");  // Reset
  QString messagePattern = QStringLiteral(
      "[%{time yyyyMMdd h:mm:ss.zzz t} "
      "%{if-debug}D%{endif}"     // debug
      "%{if-info}I%{endif}"      // info
      "%{if-warning}W%{endif}"   // warning
      "%{if-critical}C%{endif}"  // critical
      "%{if-fatal}F%{endif}]"    // fatal
      " %{file}:%{line} - %{message}");

  messagePattern.prepend(colorCodePattern).append(resetCode);
  qSetMessagePattern(messagePattern);
}

void initLog(const QString& logPath, int logMaxCount) {
  startUpTime = QDateTime::currentDateTime();
  gLogPath = logPath;
  static QString gLogDir =
      QCoreApplication::applicationDirPath() + "/" + gLogPath;
  gLogMaxCount = logMaxCount;

  setupPattern();

  QDir dir(gLogDir);
  if (!dir.exists()) {
    dir.mkpath(dir.absolutePath());
  }

  QStringList infoList = dir.entryList(QDir::Files, QDir::Name);
  while (infoList.size() > gLogMaxCount) {
    dir.remove(infoList.first());
    infoList.removeFirst();
  }

  QString fileNameDt = startUpTime.toString("yyyy-MM-dd_hh_mm_ss");
  QString newfileName = QString("%1/%2_log.txt").arg(gLogDir).arg(fileNameDt);

  file.setFileName(newfileName);
  file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);

  qInstallMessageHandler(outputMessage);
}

static void outputMessage(QtMsgType type, const QMessageLogContext& context,
                          const QString& msg) {
  QString message = qFormatLogMessage(type, context, msg.trimmed());

  static QMutex mutex;
  static QTextStream textStream;

  mutex.lock();
  if (file.isOpen()) {
    textStream.setDevice(&file);

    textStream.setEncoding(QStringConverter::Utf8);

    textStream << message << Qt::endl;
    textStream.flush();
  }
  mutex.unlock();
}
}  // namespace Logger
