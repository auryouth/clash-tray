import os
import subprocess
import sys
import webbrowser

import psutil
import pystray
from PIL import Image
from pystray import Menu
from pystray import MenuItem as item
from win11toast import toast


def resource_path(relative_path):
    """Get absolute path to resource, works for dev and for PyInstaller"""
    try:
        # PyInstaller creates a temp folder and stores path in _MEIPASS
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")
    return os.path.join(base_path, relative_path)


def notify(info):
    icon_path = resource_path("Meta.ico")
    toast("Clash Tray", info, icon=icon_path, duration="short")


def get_clash_status():
    this = psutil.Process()
    for child in this.children(recursive=True):
        if "clash" in child.name().lower():
            return True
    return False


def start_clash(icon, item):
    try:
        if not get_clash_status():
            home_dir = os.environ.get("USERPROFILE", "")
            subprocess.Popen(
                ["sudo", "clash"],
                creationflags=subprocess.CREATE_NO_WINDOW,
            )
            notify("Clash 启动成功！")
        else:
            notify("Clash 已经在运行了！")
            return
    except Exception as e:
        print(f"启动 Clash 出错：{e}")
        notify("启动 Clash 出错！")


def stop_clash(icon, item):
    try:
        if get_clash_status():
            subprocess.Popen(["sudo", "taskkill", "/F", "/IM", "clash.exe"])
            notify("Clash 关闭成功！")
        else:
            notify("Clash 已经关闭了！")
            return
    except Exception as e:
        print(f"关闭 Clash 出错：{e}")
        notify("关闭 Clash 出错！")


def open_webpage(icon, item):
    webbrowser.open("https://d.metacubex.one")


def exit_program(icon, item):
    stop_clash(icon, None)
    icon.stop()


def create_tray_icon():
    image_path = resource_path("Meta.png")
    image = Image.open(image_path)
    menu = Menu(
        item("启动 Clash", start_clash),
        item("打开网页", open_webpage, default=True),
        item("停止 Clash", stop_clash),
        item("退出程序", exit_program),
    )
    icon = pystray.Icon("Clash Meta", image, "Clash Meta", menu)
    icon.run()


if __name__ == "__main__":
    create_tray_icon()
