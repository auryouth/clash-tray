#ifndef CLASHTRAY_HPP
#define CLASHTRAY_HPP

#include <qcontainerfwd.h>
#include <qtmetamacros.h>

#include <QAction>
#include <QIcon>
#include <QProcess>
#include <QSystemTrayIcon>

class ClashTray : public QSystemTrayIcon {
  Q_OBJECT

 public:
  explicit ClashTray();

 private slots:

  void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
  void onDoubleClickTimeout();
  void toggle();
  void exitApplication();
  void updateTray(bool running = false);
  void startClashProcess();
  void safeStopClashProcess();

 private:  // NOLINT
  static QStringList clashCommands;
  static QString clashConfigPath;
  static QString boardUrl;
  static QString clashCommand;
  static QString clashVersion;

  QIcon trayIconNormal;
  QIcon trayIconRunning;
  QAction* toggleAction;
  QAction* openDashboardAction;
  QAction* openDirAction;
  QAction* exitAction;
  QMenu* trayIconMenu;
  QProcess* pclash = nullptr;

  auto checkAndFetchClashVersion() -> bool;
  void createActions();
  void createTrayIcon();
};

#endif  // CLASHTRAY_HPP
