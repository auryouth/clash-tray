import os
import subprocess
import sys
import webbrowser

import pystray
from PIL import Image
from pystray import Menu
from pystray import MenuItem as item
from win11toast import toast


class ClashTray:
    def __init__(self):
        self.tray_icon_path = self._resource_path("Meta.ico")
        self.icon = self._create_tray_icon()
        self.clash_process = None

    def _resource_path(self, relative_path):
        """Get absolute path to resource, works for dev and for PyInstaller"""
        try:
            # PyInstaller creates a temp folder and stores path in _MEIPASS
            base_path = sys._MEIPASS
        except Exception:
            base_path = os.path.abspath(".")
        return os.path.join(base_path, relative_path)

    def _notify(self, info):
        toast("Clash Tray", info, icon=self.tray_icon_path, duration="short")

    def _start_clash(self):
        try:
            if self.clash_process is not None and self.clash_process.poll() is not None:
                self.clash_process = None
            if self.clash_process is None:
                clash_process = subprocess.Popen(
                    ["clash"],
                    creationflags=subprocess.CREATE_NO_WINDOW,
                )
                self.clash_process = clash_process
                self._notify("Clash 启动成功！")
            else:
                self._notify("Clash 已经在运行了！")
        except Exception as e:
            print(f"启动 Clash 出错：{e}")
            self._notify("启动 Clash 出错！")

    def _stop_clash(self):
        try:
            if self.clash_process is not None and self.clash_process.poll() is None:
                self.clash_process.terminate()
                self.clash_process.wait()
                self.clash_process = None
                self._notify("Clash 关闭成功！")
            else:
                self._notify("Clash 已经关闭了！")
        except Exception as e:
            print(f"关闭 Clash 出错：{e}")
            self._notify("关闭 Clash 出错！")

    def _open_webpage(self):
        webbrowser.open("https://d.metacubex.one")

    def _exit_program(self, icon):
        self._stop_clash()
        icon.stop()

    def _create_tray_icon(self):
        tray_icon = Image.open(self.tray_icon_path)
        menu = Menu(
            item("启动 Clash", self._start_clash),
            item("打开网页", self._open_webpage, default=True),
            item("停止 Clash", self._stop_clash),
            item("退出程序", self._exit_program),
        )
        icon = pystray.Icon("Clash Meta", tray_icon, "Clash Meta", menu)
        return icon

    def run(self):
        self.icon.run()


if __name__ == "__main__":
    app = ClashTray()
    app.run()
