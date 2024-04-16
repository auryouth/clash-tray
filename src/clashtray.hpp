#ifndef CLASHTRAY_HPP
#define CLASHTRAY_HPP

#include <qcontainerfwd.h>
#include <qtmetamacros.h>

#include <QAction>
#include <QIcon>
#include <QPointer>
#include <QProcess>
#include <QSystemTrayIcon>

#include "utils/clash_process_mgr.hpp"

class ClashTray : public QSystemTrayIcon {
  Q_OBJECT

 public:
  explicit ClashTray();
  enum MyActivationReason {
    Unknown = QSystemTrayIcon::Unknown,
    Context = QSystemTrayIcon::Context,
    DoubleClick = QSystemTrayIcon::DoubleClick,
    Trigger = QSystemTrayIcon::Trigger,
    MiddleClick = QSystemTrayIcon::MiddleClick
  };
  Q_ENUM(MyActivationReason)

 private slots:

  void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
  void exitApplication();
  void updateTray(bool running = false);
  void startClashManager();
  // void safeStopClashProcess();

 private:  // NOLINT
  QStringList clashCommands;
  QString clashConfigPath;
  QString boardUrl;
  QString clashCommand;
  QString clashVersion;
  QIcon trayIconNormal;
  QIcon trayIconRunning;

  QAction* toggleAction;
  QAction* openDashboardAction;
  QAction* openDirAction;
  QAction* exitAction;
  QMenu* trayIconMenu;
  QPointer<ClashProcessManager> cManager = nullptr;

  void initClashConfig();
  void createActions();
  void createTrayIcon();
  auto checkAndFetchClashVersion() -> bool;
};

#endif  // CLASHTRAY_HPP
