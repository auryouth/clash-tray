import os
import re
import sys
import webbrowser
from subprocess import CREATE_NO_WINDOW, PIPE
from typing import Union

import psutil
from PySide6.QtCore import Qt, QTimer, Slot
from PySide6.QtGui import QAction, QIcon
from PySide6.QtWidgets import QApplication, QMenu, QSystemTrayIcon


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

        self._process: Union[psutil.Popen, None] = None
        self._last_click_reason: Union[QSystemTrayIcon.ActivationReason, None] = None

        self._clash_version = self.get_clash_version()

        self.update_tooltip()

        self.show()

    def resource_path(self, relative_path: str) -> str:
        if hasattr(sys, "_MEIPASS"):
            return os.path.join(sys._MEIPASS, relative_path)
        return os.path.join(os.path.abspath("."), relative_path)

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
        if self._last_click_reason == QSystemTrayIcon.DoubleClick:
            return
        self.open_dashboard()

    def on_double_click(self):
        self.toggle()

    def start(self):
        if self._process:
            self.showMessage(
                "clash",
                "clash 已经在运行了",
                QSystemTrayIcon.Warning,
                1000,
            )
            return
        try:
            self._process = psutil.Popen(
                "clash", stdout=PIPE, creationflags=CREATE_NO_WINDOW
            )
            gone, _ = psutil.wait_procs([self._process], timeout=1)
            if self._process in gone:
                err = self._process.stdout.read().decode()
                self._process = None
                raise Exception(err)
            else:
                self.update_tray_icon(True)
                self.update_tooltip("on")
        except Exception as e:
            self.showMessage(
                "clash",
                f"clash 启动失败,错误信息：{e}",
                QSystemTrayIcon.Critical,
                2000,
            )

    def stop(self):
        if self._process is None:
            self.showMessage(
                "clash",
                "clash 已经关闭了",
                QSystemTrayIcon.Warning,
                1000,
            )
            return
        try:
            if not psutil.pid_exists(self._process.pid):
                pass
            else:
                self._process.terminate()
                _, alive = psutil.wait_procs([self._process], timeout=2)
                if self._process in alive:
                    self._process = self._process.kill()
            self._process = None
            self.update_tray_icon(False)
            self.update_tooltip("off")
        except Exception as e:
            self.showMessage(
                "clash",
                f"clash 关闭失败，错误信息： {e}",
                QSystemTrayIcon.Critical,
                2000,
            )

    def toggle(self):
        if self._process:
            self.stop()
        else:
            self.start()

    def open_dashboard(self):
        webbrowser.open("https://d.metacubex.one")

    def open_dir(self):
        try:
            user_profile = os.getenv("USERPROFILE")
            config_path = os.path.join(user_profile, ".config", "mihomo")
            if not os.path.exists(config_path):
                raise Exception(f"目录不存在: {config_path}")
            psutil.Popen(["explorer", config_path])
        except Exception as e:
            self.showMessage(
                "clash",
                f"{e}",
                QSystemTrayIcon.Critical,
                2000,
            )

    def exit(self):
        self.stop()
        qApp.quit()

    def update_tray_icon(self, is_running: bool):
        if is_running:
            self.setIcon(self._tray_icon_running)
        else:
            self.setIcon(self._tray_icon_normal)

    def get_clash_version(self) -> str:
        try:
            process = psutil.Popen(["clash", "-v"], stdout=PIPE)
            output, _ = process.communicate()
            version_line = output.decode().splitlines()[0]
            match = re.search(r"v(\d+\.\d+\.\d+)", version_line)
            if match:
                return match.group(1)
            else:
                return ""
        except Exception as e:
            return ""

    def update_tooltip(self, tun_mode: str = "off"):
        tooltip = f"Clash Meta {self._clash_version}\nTun 模式: {tun_mode}"
        self.setToolTip(tooltip)

    def create_actions(self):
        self._toggle_action = QAction("切换", self)
        self._toggle_action.triggered.connect(self.toggle, Qt.UniqueConnection)

        self._start_action = QAction("启动", self)
        self._start_action.triggered.connect(self.start, Qt.UniqueConnection)

        self._stop_action = QAction("关闭", self)

        self._stop_action.triggered.connect(self.stop, Qt.UniqueConnection)

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
        self._tray_icon_menu.addAction(self._start_action)
        self._tray_icon_menu.addAction(self._stop_action)
        self._tray_icon_menu.addAction(self._open_dashboard_action)
        self._tray_icon_menu.addAction(self._open_dir_action)
        self._tray_icon_menu.addSeparator()
        self._tray_icon_menu.addAction(self._exit_action)

        self.setContextMenu(self._tray_icon_menu)
