# AltTaber

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
- 可用于隐藏`AltTaber`窗口（若可见）

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

### 4. 在`任务栏`的应用图标上使用`🖱️鼠标滚轮`切换窗口. 🚧`[Beta]`🚧

> [!CAUTION]
> 实验性功能，仍在测试中，可能会遇到一些问题
>
> 例如：对某些应用程序失效

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
- 更合理的窗口过滤规则
- 支持高DPI缩放、多屏幕显示
- 可切换显示屏幕：跟随鼠标所在屏幕 / 主屏幕

## ✒️TO-DO

- [x] 单例模式
- [ ] 自适应app太多的情况
- [x] 支持配置开启启动项
- [x] 支持管理管理员权限窗口
- [ ] 自定义配置
- [ ] ...

## 🔑以管理员身份运行（可选）

`AltTaber`可以在普通用户权限下运行，但有一定的局限性：

只有以**管理员身份**运行，`AltTaber`才能管理拥有更高权限的窗口，例如：

- 系统窗口：如任务管理器
- 管理员权限窗口：如游戏加速器

### 开机自启动

在托盘菜单中点击`Start with Windows`即可设置开机自启动，自启动时的权限与当前程序权限一致

- 若当前程序以**管理员**权限运行，则开机自启动也会以管理员权限运行（计划任务）🔑
- 若当前程序以**普通**权限运行，则开机自启动也会以普通权限运行（注册表）

## ⚙配置

配置保存在`config.ini`文件中，有两种方式可以修改配置：

1. 直接修改程序目录下的`config.ini`文件，并重启程序
2. 🌟\[推荐\] 使用托盘菜单（右键托盘图标）中的`Settings`选项，此时会自动用`notepad`打开配置文件；
   修改后，保存并**关闭文件**，程序会自动重载配置

### 配置项

#### 字体

```ini
[label]
font_family = "Microsoft YaHei UI"
font_size = 10
```

## 🧐Reference

- [window-switcher](https://github.com/sigoden/window-switcher)
- [cmdtab](https://github.com/stianhoiland/cmdtab)