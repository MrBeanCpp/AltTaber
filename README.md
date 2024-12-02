﻿# AltTaber

![GitHub release (latest by date)](https://img.shields.io/github/v/release/MrBeanCpp/AltTaber)
![Github Release Downloads](https://img.shields.io/github/downloads/MrBeanCpp/AltTaber/total)
![Language](https://img.shields.io/badge/language-C++-239120)
![OS](https://img.shields.io/badge/OS-Windows-0078D4)

一款 MacOS 风格的 `Alt-Tab` 窗口/应用切换器，专为`Windows`💻️开发:

![ui](img/ui.png)

## ⚡主要功能

### 1. ``` Alt+Tab ```: 在应用程序📦间切换

- 按住`Shift`反向切换
- 松开`Alt`后，切换到指定应用
- **最近使用**的应用优先排在左侧

![switch apps](img/Alt_tab.gif)

### 2. ``` Alt+` ```: 在同一应用的不同**窗口**🪟间轮换

- 按住`Shift`反向切换
- **最近活动**的窗口优先访问

![switch windows](img/Alt_`.gif)

### 3. `Alt+Tab` 呼出窗口切换器后，可用`🖱️鼠标滚轮`切换指定应用的窗口

- 向上滚轮：切换到上一个窗口（不改变焦点）
- 向下滚轮：**最小化**上一个窗口

#### ⌨️支持键盘操作

- 方向键:
    - ⬅️ ➡️: 切换当前选中的应用
    - ⬆️ ⬇️: 映射到滚轮上下，切换/最小化窗口
- 支持`Vim`风格的快捷键:
    - `h` `l`: 切换当前选中的应用
    - `j` `k`: 切换/最小化窗口

![wheel](img/Alt_Wheel.gif)

### 4. 在`任务栏`的应用图标上使用`🖱️鼠标滚轮`切换窗口

- 向上滚轮：切换到上一个窗口
- 向下滚轮：**最小化**上一个窗口

![taskbar wheel](img/Taskbar_Wheel.gif)

## 🌟更多特色

- 窗口背景**毛玻璃**特效
    - ![bg blur](img/bg-blur.png)
- 适配`Win11`的窗口圆角效果
- 在应用图标右上角显示**窗口数量**（Badge）
    - ![app badge](img/app%20badge.png)
- 在`QQ`🐧图标右下角显示当前聊天好友`头像`
    - ![qq avatar](img/app%20qq%20avatar.png)
- 支持在更高权限窗口上使用`Alt+Tab`切换

## ✒️TO-DO

- [x] 高DPI适配
- [ ] 多屏幕支持
- [ ] 单例模式
- [ ] 自适应app太多的情况
- [ ] 支持配置开启启动项
- [ ] 支持管理管理员权限窗口
- [ ] 自定义配置
- [ ] ...

## 🧐Reference

- [window-switcher](https://github.com/sigoden/window-switcher)
- [cmdtab](https://github.com/stianhoiland/cmdtab)