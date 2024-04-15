/*
 * FROM https://github.com/jaredtao/TaoLogger
 */

#include "utils/logger.hpp"

#include <debugapi.h>
#include <qcontainerfwd.h>
#include <qlogging.h>
#include <qstringconverter_base.h>
#include <qtpreprocessorsupport.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QMutex>
#include <QTextStream>
#include <QtTypes>
#include <array>
#include <cstddef>

#include "utils/logger_template.hpp"

namespace Logger {
static QString gLogDir;
static int gLogMaxCount;

static void outputMessage(QtMsgType type, const QMessageLogContext& context,
                          const QString& msg);
static void outputMessageAsync(QtMsgType type,
                               const QMessageLogContext& context,
                               const QString& msg);

void initLog(const QString& logPath, int logMaxCount, bool async) {
  if (async) {
    qInstallMessageHandler(outputMessageAsync);
  } else {
    qInstallMessageHandler(outputMessage);
  }

  gLogDir = QCoreApplication::applicationDirPath() + "/" + logPath;
  gLogMaxCount = logMaxCount;
  QDir dir(gLogDir);
  if (!dir.exists()) {
    dir.mkpath(dir.absolutePath());
  }
  QStringList infoList = dir.entryList(QDir::Files, QDir::Name);
  while (infoList.size() > gLogMaxCount) {
    dir.remove(infoList.first());
    infoList.removeFirst();
  }
}

static void outputMessageAsync(QtMsgType type,
                               const QMessageLogContext& context,
                               const QString& msg) {
  static const QString messageTemp = QString("<div class=\"%1\">%2</div>\r\n");
  static const std::array<char, 5> typeList = {'d', 'w', 'c', 'f', 'i'};
  static QMutex mutex;
  static QFile file;
  static QTextStream textStream;
  static uint count = 0;
  static const uint maxCount = 512;
  Q_UNUSED(context);
  QDateTime datetime = QDateTime::currentDateTime();
  // 每小时一个文件
  QString fileNameDt = datetime.toString("yyyy-MM-dd_hh");

  // 每分钟一个文件
  // QString fileNameDt = dt.toString("yyyy-MM-dd_hh_mm");

  QString contentDt = datetime.toString("yyyy-MM-dd hh:mm:ss");
  QString message = QString("%1 %2").arg(contentDt).arg(msg);
  QString htmlMessage =
      messageTemp.arg(typeList[static_cast<size_t>(type)]).arg(message);
  QString newfileName = QString("%1/%2_log.html").arg(gLogDir).arg(fileNameDt);
  mutex.lock();
  if (file.fileName() != newfileName) {
    if (file.isOpen()) {
      file.close();
    }
    file.setFileName(newfileName);
    bool exist = file.exists();
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    textStream.setDevice(&file);

    textStream.setEncoding(QStringConverter::Utf8);
    if (!exist) {
      textStream << logTemplate << "\r\n";
    }
  }
  textStream << htmlMessage;
  textStream.flush();
  count += static_cast<uint>(htmlMessage.length());
  if (count >= maxCount) {
    file.close();
    file.open(QIODevice::WriteOnly | QIODevice::Append);
  }
  mutex.unlock();
#ifdef Q_OS_WIN
  ::OutputDebugString(message.toStdWString().data());
  ::OutputDebugString(L"\r\n");
#else
  fprintf(stderr, message.toStdString().data());
#endif
}

static void outputMessage(QtMsgType type, const QMessageLogContext& context,
                          const QString& msg) {
  static const QString messageTemp = QString("<div class=\"%1\">%2</div>\r\n");
  static const std::array<char, 5> typeList = {'d', 'w', 'c', 'f', 'i'};
  static QMutex mutex;

  Q_UNUSED(context);
  QDateTime datetime = QDateTime::currentDateTime();

  // 每小时一个文件
  QString fileNameDt = datetime.toString("yyyy-MM-dd_hh");

  // 每分钟一个文件
  // QString fileNameDt = dt.toString("yyyy-MM-dd_hh_mm");

  QString contentDt = datetime.toString("yyyy-MM-dd hh:mm:ss");
  QString message = QString("%1 %2").arg(contentDt).arg(msg);
  QString htmlMessage =
      messageTemp.arg(typeList[static_cast<size_t>(type)]).arg(message);
  QFile file(QString("%1/%2_log.html").arg(gLogDir).arg(fileNameDt));
  mutex.lock();

  bool exist = file.exists();
  file.open(QIODevice::WriteOnly | QIODevice::Append);
  QTextStream textStream(&file);
  textStream.setEncoding(QStringConverter::Utf8);
  if (!exist) {
    textStream << logTemplate << "\r\n";
  }
  textStream << htmlMessage;
  file.close();
  mutex.unlock();
#ifdef Q_OS_WIN
  ::OutputDebugString(message.toStdWString().data());
  ::OutputDebugString(L"\r\n");
#else
  fprintf(stderr, message.toStdString().data());
#endif
}
}  // namespace Logger
