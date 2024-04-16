#include "clash_process_mgr.hpp"

#include <qcontainerfwd.h>
#include <qtmetamacros.h>

#include <QApplication>
#include <QObject>
#include <QProcess>
#include <QTime>

#include "utils/logger.hpp"

ClashProcessManager::ClashProcessManager(QObject* parent) : QObject(parent) {
  pclash = new QProcess(this);
}

void ClashProcessManager::testClashConfig(const QString& command,
                                          const QStringList& arguments) {
  LOG_INFO << __FUNCTION__;

  QProcess test;

  test.start(command, arguments);
  test.waitForFinished();

  if (test.exitCode() == 0) {
    LOG_INFO << "test clash config exitCode: " << test.exitCode();
    LOG_INFO << test.readAllStandardOutput();

    emit clashConfigTested(true);
  } else {
    QString err = test.readAllStandardOutput();
    LOG_CRITICAL << err;

    emit clashConfigTested(false, err);
  }
}

void ClashProcessManager::startClashProcess(const QString& command,
                                            const QStringList& arguments) {
  LOG_INFO << __FUNCTION__;

  connect(pclash, &QProcess::started, [this]() {
    LOG_INFO << "Clash started";
    emit pclashStarted();
  });

  connect(pclash, &QProcess::errorOccurred, [this]() {
    LOG_CRITICAL << pclash->error() << pclash->errorString();

    // emit pclashErrorOccurred(pclash->errorString());
  });

  connect(pclash, &QProcess::finished,
          [this](int exitCode, QProcess::ExitStatus exitStatus) {
            LOG_INFO << "Clash finished with exit code: " << exitCode
                     << " and exit status: " << exitStatus;

            emit pclashFinished();
          });

  pclash->start(command, arguments);
}
