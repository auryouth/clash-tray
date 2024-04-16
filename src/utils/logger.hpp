#pragma once
#include <qlogging.h>

#include <QString>

namespace Logger {

#define LOG_DEBUG qDebug().noquote()
#define LOG_INFO qInfo().noquote()
#define LOG_WARNING qWarning().noquote()
#define LOG_CRITICAL qCritical().noquote()
#define LOG_FATAL qFatal().noquote()

static QString gLogPath = "Log";

static int gLogMaxCount = 10;

void initLog(const QString& logPath = gLogPath, int logMaxCount = gLogMaxCount);

}  // namespace Logger
