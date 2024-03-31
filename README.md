# 一个管理clash-meta裸核的windows tray工具

## 打包命令
```bash
pyinstaller --onefile --uac-admin --icon=Meta.ico --noconsole --add-data "Meta.ico;." --add-data "Meta-running.ico;." clash-tray.py
```

## 缺陷
- 多个实例可以同时启动