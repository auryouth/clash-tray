#include "clashtray.hpp"

#include <qcontainerfwd.h>

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QForeach>
#include <QIcon>
#include <QMenu>
#include <QProcess>
#include <QRegularexpression>
#include <QSystemTrayIcon>
#include <QTimer>

#include "utils/logger.hpp"
#include "utils/utils.hpp"

QStringList ClashTray::clashCommands = {
    "clash-meta-alpha", "clash-alpha", "mihomo-alpha",
    "clash-meta",       "clash",       "mihomo"};
QString ClashTray::clashConfigPath = QDir::homePath() + "/.config/clash";
QString ClashTray::boardUrl = "https://d.metacubex.one";
QString ClashTray::clashCommand = "Clash 未安装";
QString ClashTray::clashVersion = "版本未知";

ClashTray::ClashTray() {
  trayIconNormal = QIcon(":/resources/meta_normal.ico");
  trayIconRunning = QIcon(":/resources/meta_running.ico");
  setIcon(trayIconNormal);

  connect(this, &QSystemTrayIcon::activated, this,
          &ClashTray::onTrayIconActivated);

  createActions();
  createTrayIcon();

  show();

  checkAndFetchClashVersion();

  updateTray(false);
}

void ClashTray::createActions() {
  LOG_INFO << __FUNCTION__;

  toggleAction = new QAction("Toggle", this);
  connect(toggleAction, &QAction::triggered, this, &ClashTray::toggle);

  openDashboardAction = new QAction("Open Dashboard", this);
  connect(openDashboardAction, &QAction::triggered,
          []() { Utils::openUrl(boardUrl); });

  openDirAction = new QAction("Open Directory", this);
  connect(openDirAction, &QAction::triggered, [this]() {
    QString msg;
    bool exists = Utils::dirExists(clashConfigPath, msg);
    if (!exists) {
      LOG_WARN << msg;
      showMessage("打开目录失败", msg, QSystemTrayIcon::Warning, 2000);
    };
    LOG_INFO << msg;
    Utils::openDir(clashConfigPath);
  });

  exitAction = new QAction("Exit", this);
  connect(exitAction, &QAction::triggered, this, &ClashTray::exitApplication);
}

void ClashTray::createTrayIcon() {
  LOG_INFO << __FUNCTION__;

  trayIconMenu = new QMenu();
  trayIconMenu->addAction(toggleAction);
  trayIconMenu->addAction(openDashboardAction);
  trayIconMenu->addAction(openDirAction);
  trayIconMenu->addSeparator();
  trayIconMenu->addAction(exitAction);
  setContextMenu(trayIconMenu);
}

// XXX: Workaround -> Win11 bug -> QSystemTrayIcon 目前无法触发双击事件
void ClashTray::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
  if (reason == QSystemTrayIcon::Trigger) {
    this->setProperty("lastActivateReason", reason);
    QTimer::singleShot(QApplication::doubleClickInterval(), this,
                       &ClashTray::onDoubleClickTimeout);
  } else if (reason == QSystemTrayIcon::DoubleClick) {
    LOG_INFO << "Double clicked";
    this->setProperty("lastActivateReason", reason);
    toggle();
  }
}

void ClashTray::onDoubleClickTimeout() {
  if (this->property("lastActivateReason") != QSystemTrayIcon::DoubleClick) {
    LOG_INFO << this->property("lastActivateReason");

    Utils::openUrl(boardUrl);
  }
}

void ClashTray::toggle() {
  LOG_INFO << __FUNCTION__;
  if (pclash == nullptr || pclash->state() == QProcess::NotRunning) {
    startClashProcess();
  } else {
    safeStopClashProcess();
  }
}

void ClashTray::exitApplication() {
  LOG_INFO << __FUNCTION__;
  safeStopClashProcess();

  qApp->quit();
}

auto ClashTray::checkAndFetchClashVersion() -> bool {
  LOG_INFO << __FUNCTION__;
  bool isClashFound = false;

  LOG_INFO << "Checking regex validity";
  QRegularExpression regex(R"((\bv\d+\.\d+\.\d+\b|\balpha-[0-9a-f]+\b))");
  bool regexValid = true;
  if (!regex.isValid()) {
    clashVersion = "正则无效";

    LOG_CRIT << "Regex valid: " << !regexValid << " Regex: " << regex.pattern();
    LOG_CRIT << "Regex error: " << regex.errorString();
    LOG_CRIT << "Clash Version is set to 'regex invalid'";
    showMessage("Regex 无效", regex.errorString(), QSystemTrayIcon::Critical,
                2000);
    regexValid = false;
  } else {
    LOG_INFO << "Regex valid: " << regexValid << " Regex: " << regex.pattern();
  }
  auto* pcheck = new QProcess(this);

  foreach (const QString& cmd, clashCommands) {
    pcheck->start(cmd, QStringList() << "-v");
    QString command =
        pcheck->program().append(" ").append(pcheck->arguments().join(" "));
    LOG_INFO << "Checking clash with command: " << command;

    if (pcheck->waitForFinished() && pcheck->exitCode() == 0) {
      isClashFound = true;
      LOG_INFO << "Clash executable found: " << cmd;

      clashCommand = cmd;
      if (regexValid) {
        LOG_INFO << "Checking version with regex";

        QString output = pcheck->readAllStandardOutput();
        QRegularExpressionMatch match = regex.match(output);
        if (match.hasMatch()) {
          clashVersion = match.captured();
          LOG_INFO << "Regex match: " << true << " version: " << clashVersion;
        } else {
          LOG_WARN << "Regex match: " << false << " version: " << "unknown";
          LOG_WARN << "Regex text: " << output;

          showMessage("正则匹配失败", "请查看日志详情",
                      QSystemTrayIcon::Warning, 2000);
        }
      }
      break;
    }

    LOG_WARN << "Check error: " << pcheck->error();
    LOG_WARN << pcheck->errorString();
  };

  if (!isClashFound) {
    showMessage("未检测到 Clash 可执行程序", "详情请查看日志",
                QSystemTrayIcon::Critical, 2000);
  }

  return isClashFound;
}

void ClashTray::startClashProcess() {
  LOG_INFO << __FUNCTION__;
  if (!checkAndFetchClashVersion()) {
    return;
  }

  pclash = new QProcess(this);

  connect(pclash, &QProcess::started, [this]() {
    QTimer::singleShot(500, [this]() {
      if (pclash->state() == QProcess::Running) {
        LOG_INFO << "Clash started";
        this->updateTray(true);
      }
    });
  });

  connect(pclash, &QProcess::errorOccurred, [this]() {
    LOG_CRIT << "Clash error type: " << pclash->error();
    LOG_CRIT << "Clash error: " << pclash->errorString();
  });
  connect(pclash, &QProcess::finished, [this]() {
    // HACK clash 启动失败不会触发 errorOccurred
    //  无法从 readAllStandardError() 或者 errorString() 获取错误信息
    // 目前观察只有启动失败会返回 UnknownError Type
    if (pclash->error() == QProcess::UnknownError) {
      LOG_CRIT << "Clash start failed";
      QString errorMessage = QString(pclash->readAll());
      LOG_CRIT << errorMessage;
      showMessage("Clash 启动失败", errorMessage, QSystemTrayIcon::Critical,
                  2000);
    } else if (pclash->exitStatus() == QProcess::CrashExit) {
      LOG_CRIT << "Clash crash exit";
      LOG_CRIT << pclash->readAllStandardError();
    } else {
      LOG_INFO << "Clash finished";
    }

    updateTray(false);
    pclash->deleteLater();
  });

  pclash->start(clashCommand, QStringList() << "-d" << clashConfigPath);
}

void ClashTray::safeStopClashProcess() {
  LOG_INFO << __FUNCTION__;
  if (pclash != nullptr && pclash->state() == QProcess::Running) {
    pclash->close();
  }
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
  LOG_INFO << "Tray updated: " << toolTip() << " running: " << running;
}
