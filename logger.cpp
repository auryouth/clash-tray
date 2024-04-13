#include "logger.hpp"

#include <qlogging.h>
#include <qstringliteral.h>

#include <QDateTime>
#include <QHash>
#include <QIODevice>
#include <QObject>
#include <QString>

QFile* Logger::logFile = nullptr;
bool Logger::isInit = false;
QHash<QtMsgType, QString> Logger::contextNames = {
    {QtMsgType::QtDebugMsg, " Debug  "},
    {QtMsgType::QtInfoMsg, "  Info  "},
    {QtMsgType::QtWarningMsg, "Warning "},
    {QtMsgType::QtCriticalMsg, "Critical"},
    {QtMsgType::QtFatalMsg, " Fatal  "}};

void Logger::init() {
  if (isInit) {
    return;
  }

  // Create log file
  logFile = new QFile;
  logFile->setFileName("./Log.log");
  logFile->open(QIODevice::Append | QIODevice::Text);

  // Redirect logs to messageOutput
  qInstallMessageHandler(messageOutput);

  // Clear file contents
  logFile->resize(0);

  isInit = true;
}

void Logger::clean() {
  if (logFile != nullptr) {
    logFile->close();
    logFile->deleteLater();
  }
}

void Logger::messageOutput(QtMsgType type, const QMessageLogContext& context,
                           const QString& msg) {
  QString filename = QString(context.file).section('/', -1);

  QString log =
      QObject::tr("%1 | %2 | %3 | %4 | %5 | %6\n")
          .arg(QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss"),
               -20)
          .arg(contextNames.value(type), -8)
          .arg(context.line, -3, 10, QChar(' '))
          .arg(filename, -15)
          .arg(QString(context.function), -50)
          .arg(msg);

  logFile->write(log.toLocal8Bit());
  logFile->flush();
}
