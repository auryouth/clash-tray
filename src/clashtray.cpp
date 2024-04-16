#include "clashtray.hpp"

#include <qcontainerfwd.h>
#include <qlogging.h>

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QMenu>
#include <QMetaEnum>
#include <QSystemTrayIcon>
#include <QTimer>

#include "utils/clash_process_mgr.hpp"
#include "utils/logger.hpp"
#include "utils/utils.hpp"

ClashTray::ClashTray() {
  trayIconNormal = QIcon(":/resources/meta_normal.ico");
  trayIconRunning = QIcon(":/resources/meta_running.ico");
  setIcon(trayIconNormal);

  connect(this, &QSystemTrayIcon::activated, this,
          &ClashTray::onTrayIconActivated);

  initClashConfig();
  createActions();
  createTrayIcon();

  show();

  checkAndFetchClashVersion();
  updateTray(false);
}

void ClashTray::initClashConfig() {
  LOG_INFO << __FUNCTION__;

  clashCommands = {"clash-meta-alpha", "clash-alpha", "mihomo-alpha",
                   "clash-meta",       "clash",       "mihomo"};
  clashConfigDir = QDir::homePath() + "/.config/clash";
  boardUrl = "https://d.metacubex.one";
  clashCommand = "Clash 未安装";
  clashVersion = "版本未知";
  clashConfigName = "config.yaml";
}

void ClashTray::createActions() {
  LOG_INFO << __FUNCTION__;

  // 1.Toggle action
  toggleAction = new QAction("Toggle", this);
  toggleAction->setCheckable(true);
  toggleAction->setChecked(false);
  connect(toggleAction, &QAction::toggled, [this](bool checked) {
    LOG_INFO << toggleAction->text() << ":" << checked;
    if (checked) {
      startClashManager();
    } else {
      if (cManager != nullptr) {
        cManager->deleteLater();
      }
    }
  });

  // 2.Open Dashboard action
  openDashboardAction = new QAction("Open Dashboard", this);
  connect(openDashboardAction, &QAction::triggered,
          [this]() { Utils::openUrl(boardUrl); });

  // 3.Open Directory action
  openDirAction = new QAction("Open Directory", this);
  connect(openDirAction, &QAction::triggered, [this]() {
    QString msg;
    bool exists = Utils::dirExists(clashConfigDir, msg);
    if (!exists) {
      showMessage("打开目录失败", msg, QSystemTrayIcon::Warning, 2000);
    } else {
      Utils::openDir(clashConfigDir);
    }
  });

  // 4.Open Config action
  openConfigAction = new QAction("Open Config", this);
  connect(openConfigAction, &QAction::triggered, [this]() {
    QString configFilePath = clashConfigDir + "/" + clashConfigName;
    QString err;
    if (!Utils::fileExists(configFilePath, err)) {
      showMessage("配置文件不存在", configFilePath, QSystemTrayIcon::Warning,
                  2000);
      return;
    }

    Utils::openFile(configFilePath);
  });

  // 5.Exit action
  exitAction = new QAction("Exit", this);
  connect(exitAction, &QAction::triggered, this, &ClashTray::exitApplication);
}

void ClashTray::createTrayIcon() {
  LOG_INFO << __FUNCTION__;

  trayIconMenu = new QMenu();
  trayIconMenu->addAction(toggleAction);
  trayIconMenu->addAction(openDashboardAction);
  trayIconMenu->addAction(openDirAction);
  trayIconMenu->addAction(openConfigAction);
  trayIconMenu->addSeparator();
  trayIconMenu->addAction(exitAction);
  setContextMenu(trayIconMenu);
}

// XXX: Workaround -> Win11 bug -> QSystemTrayIcon 目前无法触发双击事件
void ClashTray::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
  QMetaEnum metaEnum = QMetaEnum::fromType<MyActivationReason>();
  LOG_INFO << __FUNCTION__ << metaEnum.valueToKey(reason);
  this->setProperty("lastActivateReason", reason);

  if (reason == QSystemTrayIcon::Trigger) {
    QTimer::singleShot(QApplication::doubleClickInterval(), [this] {
      if (this->property("lastActivateReason") == QSystemTrayIcon::Trigger) {
        Utils::openUrl(boardUrl);
      }
    });
  } else if (reason == QSystemTrayIcon::DoubleClick) {
    toggleAction->toggle();
  }
}

void ClashTray::exitApplication() {
  if (cManager != nullptr) {
    cManager->deleteLater();
  }
  LOG_INFO << __FUNCTION__;

  qApp->quit();
}

auto ClashTray::checkAndFetchClashVersion() -> bool {
  LOG_INFO << __FUNCTION__;

  const QString regexPattern = R"((\bv\d+\.\d+\.\d+\b|\balpha-[0-9a-f]+\b))";

  // check clash executable
  auto [isClashFound, result] = Utils::commandFind(clashCommands);
  if (!isClashFound) {
    LOG_CRITICAL << "Clash executable not found";
    showMessage("未检测到 Clash 可执行程序", result, QSystemTrayIcon::Critical,
                2000);
    return isClashFound;
  }

  clashCommand = result;

  // check and get clash version outputs
  QString vcommand = clashCommand + " -v";
  auto [vcommandValid, vresult] = Utils::commandCheck(vcommand);
  if (!vcommandValid) {
    showMessage("Clash 版本命令无效", vresult, QSystemTrayIcon::Critical, 2000);
  }

  // get version from output with regex
  if (vcommandValid) {
    auto [isMatch, regexResult] = Utils::regexMatch(regexPattern, vresult);
    if (!isMatch) {
      clashVersion = "版本未知";
      showMessage("正则匹配失败", regexResult, QSystemTrayIcon::Warning, 2000);
    } else {
      clashVersion = regexResult;
    }
  }

  return isClashFound;
}

void ClashTray::startClashManager() {
  LOG_INFO << __FUNCTION__;

  QString err;
  if (!Utils::dirExists(clashConfigDir, err)) {
    showMessage("Clash 配置目录不存在", err, QSystemTrayIcon::Critical, 2000);
    toggleAction->setChecked(false);
    return;
  }

  if (!checkAndFetchClashVersion()) {
    toggleAction->setChecked(false);
    return;
  }

  cManager = new ClashProcessManager(this);
  QStringList runArgs = {"-d", clashConfigDir};
  QStringList testArgs = {"-d", clashConfigDir, "-t"};

  connect(cManager, &ClashProcessManager::pclashStarted, this,
          [this]() { updateTray(true); });

  connect(cManager, &ClashProcessManager::pclashFinished, this, [this]() {
    updateTray(false);
    toggleAction->setChecked(false);
  });

  // FIXME: windows 任务管理器关闭clash不会error, 程序内部终止会error
  // XXX: 有待区分主动和被动关闭的情况
  // connect(cManager, &ClashProcessManager::pclashErrorOccurred,
  //         [this](const QString& errorString) {
  //           showMessage("Clash 发生错误", errorString,
  //                       QSystemTrayIcon::Critical, 2000);
  //         });

  connect(cManager, &ClashProcessManager::clashConfigTested,
          [this, &runArgs](bool success, const QString& errorString) {
            if (!success) {
              showMessage("Clash 配置文件测试失败", errorString,
                          QSystemTrayIcon::Critical, 2000);
              toggleAction->setChecked(false);
            } else {
              cManager->startClashProcess(clashCommand, runArgs);
            }
          });

  cManager->testClashConfig(clashCommand, testArgs);
}

void ClashTray::updateTray(bool running) {
  LOG_INFO << __FUNCTION__;
  setToolTip(tr("%1 %2").arg(clashCommand, clashVersion));

  if (running) {
    setIcon(trayIconRunning);
    setToolTip(toolTip().append("\nTun 模式: on"));
  } else {
    setIcon(trayIconNormal);
    setToolTip(toolTip().append("\nTun 模式: off"));
  }
  qInfo().nospace() << "Tray updated: " << toolTip();
}
