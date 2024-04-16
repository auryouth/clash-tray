#ifndef CLASHPROCESSMANAGER_HPP
#define CLASHPROCESSMANAGER_HPP

#include <qcontainerfwd.h>
#include <qtmetamacros.h>

#include <QDebug>
#include <QObject>
#include <QPointer>
#include <QProcess>

#include "utils/logger.hpp"

class ClashProcessManager : public QObject {
  Q_OBJECT

 public:
  explicit ClashProcessManager(QObject* parent = nullptr);
  ~ClashProcessManager() {
    LOG_INFO << "ClashProcessManager destructor";
    if (pclash != nullptr) {
      LOG_INFO << "Close pclash process";
      pclash->close();
    }
  };
  void testClashConfig(const QString& command, const QStringList& arguments);
  void startClashProcess(const QString& command, const QStringList& arguments);

 signals:
  void clashConfigTested(bool success, const QString& errorString = "");
  void pclashStarted();
  // void pclashErrorOccurred(const QString& errorString);
  void pclashFinished();

 private:
  QPointer<QProcess> pclash = nullptr;
};

#endif  // CLASHPROCESSMANAGER_HPP
