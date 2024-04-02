import PyInstaller.__main__

PyInstaller.__main__.run(
    [
        "main.py",
        "--onefile",
        "--noconsole",
        "--uac-admin",
        "--clean",
        "--noconfirm",
        "--icon=icon/meta_normal.ico",
        "--add-data=icon:icon",
        "--name=clash-tray",
    ]
)
