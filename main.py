import sys

from PySide6.QtWidgets import QApplication, QMessageBox, QSystemTrayIcon

from clash_tray import ClashTray

if __name__ == "__main__":
    app = QApplication([])

    if not QSystemTrayIcon.isSystemTrayAvailable():
        QMessageBox.critical(
            None, "clash tray", "I couldn't detect any system tray on this system."
        )
        sys.exit(1)

    app.setQuitOnLastWindowClosed(False)

    tray = ClashTray(app)
    sys.exit(app.exec())
