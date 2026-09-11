# SwitchBrowser

Switch 大气层网页浏览器，使用 Switch 内置 WebKit 引擎渲染网页。

## 使用方法

### 方法一：本地编译（需要 devkitPro）

1. 安装 [devkitPro](https://github.com/devkitPro/installer)
2. 安装 Switch 开发包：`pacman -S switch-dev`
3. 编译：
```bash
export DEVKITPRO=/opt/devkitpro  # Windows: C:\devkitPro
cd SwitchBrowser
make
```
4. 将 `SwitchBrowser.nro` 复制到 SD 卡 `switch/SwitchBrowser/`

### 方法二：GitHub Actions 自动编译

1. Fork 这个仓库到你的 GitHub
2. Actions 标签页 → Build SwitchBrowser NRO → Run workflow
3. 编译完成后下载 artifact `SwitchBrowser-NRO`

## 操作说明

| 按键 | 功能 |
|------|------|
| A | 输入网址 / 搜索 |
| B | 书签列表 |
| Y | 浏览历史 |
| X | 设置 |
| + | 退出 |
| 方向键 | 列表选择 |
| A（列表中）| 打开 |

## 功能

- URL 输入（软键盘）
- 智能搜索（非 URL 文本自动搜索）
- 书签管理（保存/删除/打开）
- 浏览历史
- 设置（首页编辑）
- WebKit 全功能渲染（HTML5/CSS/JS/触摸/摇杆指针）
