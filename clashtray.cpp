#include "clashtray.hpp"

#include <qcontainerfwd.h>
#include <qlogging.h>

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QForeach>
#include <QIcon>
#include <QMenu>
#include <QProcess>
#include <QRegularExpressionMatch>
#include <QRegularexpression>
#include <QSystemTrayIcon>
#include <QTimer>

#include "utils.hpp"

QStringList ClashTray::clashCommands = {
    "clash-meta-alpha", "clash-alpha", "mihomo-alpha",
    "clash-meta",       "clash",       "mihomo"};
QString ClashTray::clashConfigPath = QDir::homePath() + "/.config/clash";
QString ClashTray::boardUrl = "https://d.metacubex.one";
QString ClashTray::clashCommand = "Clash 未安装";
QString ClashTray::clashVersion = "版本未知";

ClashTray::ClashTray() {
  trayIconNormal = QIcon(":/images/meta_normal.ico");
  trayIconRunning = QIcon(":/images/meta_running.ico");
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
  qInfo() << __func__;

  toggleAction = new QAction(tr("Toggle"), this);
  connect(toggleAction, &QAction::triggered, this, &ClashTray::toggle);

  openDashboardAction = new QAction(tr("Open Dashboard"), this);
  connect(openDashboardAction, &QAction::triggered,
          []() { Utils::openUrl(boardUrl); });

  openDirAction = new QAction(tr("Open Directory"), this);
  connect(openDirAction, &QAction::triggered, [this]() {
    QString msg;
    bool exists = Utils::dirExists(clashConfigPath, msg);
    if (!exists) {
      qWarning() << msg;
      showMessage("打开目录失败", msg, QSystemTrayIcon::Warning, 2000);
    };
    qInfo() << msg;
    Utils::openDir(clashConfigPath);
  });

  exitAction = new QAction(tr("Exit"), this);
  connect(exitAction, &QAction::triggered, this, &ClashTray::exitApplication);
}

void ClashTray::createTrayIcon() {
  qInfo() << __func__;

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
    qInfo() << "Double clicked";
    this->setProperty("lastActivateReason", reason);
    toggle();
  }
}

void ClashTray::onDoubleClickTimeout() {
  if (this->property("lastActivateReason") != QSystemTrayIcon::DoubleClick) {
    qInfo() << this->property("lastActivateReason");

    Utils::openUrl(boardUrl);
  }
}

void ClashTray::toggle() {
  qInfo() << __func__;
  if (pclash == nullptr || pclash->state() == QProcess::NotRunning) {
    startClashProcess();
  } else {
    safeStopClashProcess();
  }
}

void ClashTray::exitApplication() {
  qInfo() << __func__;
  safeStopClashProcess();

  qApp->quit();
}

auto ClashTray::checkAndFetchClashVersion() -> bool {
  qInfo() << __func__;
  bool isClashFound = false;

  qInfo() << "Checking regex validity";
  QRegularExpression regex(R"((\bv\d+\.\d+\.\d+\b|\balpha-[0-9a-f]+\b))");
  bool regexValid = true;
  if (!regex.isValid()) {
    clashVersion = "正则无效";

    qCritical() << "Regex valid: " << !regexValid
                << " Regex: " << regex.pattern();
    qCritical() << "Regex error: " << regex.errorString();
    qCritical() << "Clash Version is set to 'regex invalid'";
    showMessage("Regex 无效", regex.errorString(), QSystemTrayIcon::Critical,
                2000);
    regexValid = false;
  } else {
    qInfo() << "Regex valid: " << regexValid << " Regex: " << regex.pattern();
  }
  auto* pcheck = new QProcess(this);

  foreach (const QString& cmd, clashCommands) {
    qInfo() << "Checking clash: " << cmd;
    pcheck->start(cmd, QStringList() << "-v");

    if (pcheck->waitForFinished() && pcheck->exitCode() == 0) {
      isClashFound = true;
      qInfo() << "Clash found: " << cmd;

      clashCommand = cmd;
      if (regexValid) {
        qInfo() << "Checking version with regex";

        QString output = pcheck->readAllStandardOutput();
        QRegularExpressionMatch match = regex.match(output);
        if (match.hasMatch()) {
          clashVersion = match.captured();
          qInfo() << "Regex match: " << true << " version: " << clashVersion;
        } else {
          qWarning() << "Regex match: " << false << " version: " << "unknown";
          qWarning() << "Regex text: " << output;

          showMessage("正则匹配失败", "请查看日志详情",
                      QSystemTrayIcon::Warning, 2000);
        }
      }
      break;
    }

    QString command =
        pcheck->program().append(" ").append(pcheck->arguments().join(" "));
    qWarning() << "Check error: " << pcheck->error() << command;
    qWarning() << pcheck->errorString();
  };

  if (!isClashFound) {
    showMessage("未检测到 Clash 可执行程序", "详情请查看日志",
                QSystemTrayIcon::Critical, 2000);
  }

  return isClashFound;
}

void ClashTray::startClashProcess() {
  qInfo() << __func__;
  if (!checkAndFetchClashVersion()) {
    return;
  }

  pclash = new QProcess(this);

  connect(pclash, &QProcess::started, [this]() {
    QTimer::singleShot(500, [this]() {
      if (pclash->state() == QProcess::Running) {
        qInfo() << "Clash started";
        this->updateTray(true);
      }
    });
  });

  connect(pclash, &QProcess::errorOccurred, [this]() {
    qCritical() << "Clash error type: " << pclash->error();
    qCritical() << "Clash error: " << pclash->errorString();
  });
  connect(pclash, &QProcess::finished, [this]() {
    // HACK clash 启动失败不会触发 errorOccurred
    //  无法从 readAllStandardError() 或者 errorString() 获取错误信息
    // 目前观察只有启动失败会返回 UnknownError Type
    if (pclash->error() == QProcess::UnknownError) {
      qCritical() << "Clash start failed";
      qCritical() << pclash->readAllStandardOutput();
      showMessage("Clash 启动失败", pclash->readAllStandardOutput(),
                  QSystemTrayIcon::Critical, 2000);
    } else if (pclash->exitStatus() == QProcess::CrashExit) {
      qCritical() << "Clash crash exit";
      qCritical() << pclash->readAllStandardError();
    } else {
      qInfo() << "Clash finished";
    }

    updateTray(false);
    pclash->deleteLater();
  });

  pclash->start(clashCommand, QStringList() << "-d" << clashConfigPath);
}

void ClashTray::safeStopClashProcess() {
  qInfo() << __func__;
  if (pclash != nullptr && pclash->state() == QProcess::Running) {
    pclash->close();
  }
}

void ClashTray::updateTray(bool running) {
  qInfo() << __func__;
  setToolTip(tr("%1 %2").arg(clashCommand, clashVersion));

  if (running) {
    setIcon(trayIconRunning);
    setToolTip(toolTip().append("\nTun 模式: on"));
  } else {
    setIcon(trayIconNormal);
    setToolTip(toolTip().append("\nTun 模式: off"));
  }
  qInfo() << "Tray updated: " << toolTip() << " running: " << running;
}
