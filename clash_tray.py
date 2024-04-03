import os
import re
import shutil
import subprocess
import sys
import webbrowser
from typing import Union

import psutil
from PySide6.QtCore import Qt, QTimer, Slot
from PySide6.QtGui import QAction, QIcon
from PySide6.QtWidgets import QApplication, QMenu, QSystemTrayIcon


class ClashTrayException(Exception):
    def __init__(
        self,
        cause: str,
        msg: str,
        severity: QSystemTrayIcon.MessageIcon,
    ):
        super().__init__()
        self.cause = cause
        self.msg = msg
        self.severity = severity


class ClashTray(QSystemTrayIcon):
    def __init__(self, parent: QApplication = None):
        super().__init__(parent)
        self._tray_icon_normal_path = self.resource_path("icon/meta_normal.ico")
        self._tray_icon_running_path = self.resource_path("icon/meta_running.ico")
        self._tray_icon_normal = QIcon(self._tray_icon_normal_path)
        self._tray_icon_running = QIcon(self._tray_icon_running_path)
        self.setIcon(self._tray_icon_normal)

        self.activated.connect(self.on_tray_icon_activated)

        self.create_actions()
        self.create_tray_icon()

        self._process: Union[psutil.Process, None] = None
        self._last_click_reason: Union[QSystemTrayIcon.ActivationReason, None] = None

        self._clash_config_path: str = os.path.join(
            os.getenv("USERPROFILE"), ".config", "clash"
        )
        self.show()
        self.update_tray()

    def resource_path(self, relative_path: str) -> str:
        if hasattr(sys, "_MEIPASS"):
            return os.path.join(sys._MEIPASS, relative_path)
        return os.path.join(os.path.abspath("."), relative_path)

    def check_clash_installed(self, cause: str) -> tuple[bool, str]:
        commands_to_check = ["clash-meta-alpha", "clash-meta", "clash", "mihomo"]
        try:
            for cmd in commands_to_check:
                if shutil.which(cmd):
                    return True, cmd
            err = (
                "无法从下列命令列表中找到clash相关可执行程序：\n"
                "[" + " ".join(commands_to_check) + "]\n"
                "请检查是否已经安装clash"
            )
            raise ClashTrayException(
                cause,
                err,
                QSystemTrayIcon.Critical,
            )
        except ClashTrayException as e:
            self.showMessage(e.cause, e.msg, e.severity, 2000)
            return False, "clash未安装"

    def get_clash_version(self, cause: str) -> str:
        clash_installed = self.check_clash_installed(cause)
        if not clash_installed[0]:
            return "版本未知"
        ver_command = [clash_installed[1], "-v"]
        pclash = subprocess.Popen(
            ver_command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            creationflags=subprocess.CREATE_NO_WINDOW,
        )
        pclash.wait(1)
        try:
            if pclash.returncode != 0:
                err = (
                    "获取版本命令错误： "
                    + " ".join(ver_command)
                    + "\n"
                    + pclash.stderr.read().strip().decode()
                )
                raise ClashTrayException(cause, err, QSystemTrayIcon.Critical)
            version_line = pclash.stdout.readline().strip().decode()
            regex = r"(\bv\d+\.\d+\.\d+\b|\balpha-[0-9a-f]+\b)"
            match = re.search(regex, version_line)
            if not match or not match.group(1):
                err = (
                    "clash版本信息匹配失败\n"
                    "请检查函数" + sys._getframe().f_code.co_name + "中正则匹配\n"
                    "正则表达式：" + "".join(regex) + "\n"
                    "同时请检查程序版本信息输出：\n" + version_line
                )
                raise ClashTrayException(cause, err, QSystemTrayIcon.Critical)
            return match.group(1)
        except ClashTrayException as e:
            self.showMessage(e.cause, e.msg, e.severity, 2000)
            return "版本未知"

    @Slot(QSystemTrayIcon.ActivationReason)
    def on_tray_icon_activated(self, reason: QSystemTrayIcon.ActivationReason):
        if reason == QSystemTrayIcon.Trigger:
            self._last_click_reason = reason
            QTimer.singleShot(
                QApplication.doubleClickInterval(), self.on_double_click_timeout
            )
        elif reason == QSystemTrayIcon.DoubleClick:
            self._last_click_reason = reason
            self.on_double_click()

    def on_double_click_timeout(self):
        if self._last_click_reason != QSystemTrayIcon.DoubleClick:
            self.open_dashboard()

    def on_double_click(self):
        self.toggle()

    def toggle(self):
        try:
            if self._process is None:
                self.start_clash_process()
            elif self._process.is_running():
                self.stop_clash_process()
            else:
                self._process = None
                err = (
                    "clash子进程已经被额外操作意外关闭\n"
                    "请检查是否有其他程序关闭了clash进程\n"
                    "托盘图标状态将更新"
                )
                raise ClashTrayException(
                    sys._getframe().f_code.co_name,
                    err,
                    QSystemTrayIcon.MessageIcon.Warning,
                )
        except ClashTrayException as e:
            self.showMessage(e.cause, e.msg, e.severity, 2000)
        finally:
            self.update_tray()

    def start_clash_process(self):
        func_name = sys._getframe().f_code.co_name
        if not self.clash_config_dir_exists(func_name):
            return
        clash_installed = self.check_clash_installed(func_name)
        if not clash_installed[0]:
            return
        start_command = [clash_installed[1], "-d", self._clash_config_path]
        proc = subprocess.Popen(
            start_command,
            stdout=subprocess.PIPE,
            creationflags=subprocess.CREATE_NO_WINDOW,
        )
        try:
            gone, _ = psutil.wait_procs([proc], timeout=1)
            if proc in gone:
                err = proc.stdout.read().strip().decode()
                raise ClashTrayException(
                    " ".join(start_command), err, QSystemTrayIcon.MessageIcon.Critical
                )
            proc.stdout.close()
            self._process = psutil.Process(proc.pid)
        except ClashTrayException as e:
            self.showMessage(e.cause, e.msg, e.severity, 2000)

    def stop_clash_process(self):
        self._process.terminate()
        _, alive = psutil.wait_procs([self._process], timeout=2)
        if self._process in alive:
            self._process = self._process.kill()
        self._process = None

    def update_tray(self):
        if self._process is None:
            icon = self._tray_icon_normal
            state = "off"
        else:
            icon = self._tray_icon_running
            state = "on"

        func_name = sys._getframe().f_code.co_name
        tooltip = (
            self.check_clash_installed(func_name)[1]
            + " "
            + self.get_clash_version(func_name)
            + "\n"
            + "Tun 模式："
            + state
        )
        self.setIcon(icon)
        self.setToolTip(tooltip)

    def open_dashboard(self):
        webbrowser.open("https://d.metacubex.one")

    def clash_config_dir_exists(self, cause):
        try:
            if not os.path.exists(self._clash_config_path):
                raise ClashTrayException(
                    cause,
                    "目录不存在：" + self._clash_config_path,
                    QSystemTrayIcon.Critical,
                )
            return True
        except ClashTrayException as e:
            self.showMessage(e.cause, e.msg, e.severity, 2000)
            return False

    def open_dir(self):
        if not self.clash_config_dir_exists("打开目录失败"):
            return
        os.startfile(self._clash_config_path)

    def exit(self):
        if self._process is not None and self._process.is_running():
            self.stop_clash_process()
        qApp.quit()

    def create_actions(self):
        self._toggle_action = QAction("切换", self)
        self._toggle_action.triggered.connect(self.toggle, Qt.UniqueConnection)

        self._open_dashboard_action = QAction("打开面板", self)
        self._open_dashboard_action.triggered.connect(
            self.open_dashboard, Qt.UniqueConnection
        )

        self._open_dir_action = QAction("打开配置目录", self)
        self._open_dir_action.triggered.connect(self.open_dir, Qt.UniqueConnection)

        self._exit_action = QAction("退出", self)
        self._exit_action.triggered.connect(self.exit, Qt.UniqueConnection)

    def create_tray_icon(self):
        self._tray_icon_menu = QMenu()
        self._tray_icon_menu.addAction(self._toggle_action)
        self._tray_icon_menu.addAction(self._open_dashboard_action)
        self._tray_icon_menu.addAction(self._open_dir_action)
        self._tray_icon_menu.addSeparator()
        self._tray_icon_menu.addAction(self._exit_action)

        self.setContextMenu(self._tray_icon_menu)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    clash_tray = ClashTray(app)
    sys.exit(app.exec())
