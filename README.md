# DouyinModerator - 抖音直播房管工具

基于 Qt 6 + C++ 的抖音直播间管理工具，支持房管模式自动运营。

## 功能

- **直播间监控** — 实时捕获进入、聊天、礼物、关注、点赞、入团等事件
- **自动回复弹幕** — 规则引擎，触发条件自动发送弹幕，支持冷却防刷屏
- **定时弹幕** — 按间隔循环发送指定弹幕
- **自动点赞** — 按设定间隔自动点赞
- **抖音登录** — 内嵌浏览器扫码登录
- **表情支持** — 60+ 抖音表情短码

## 技术栈

- Qt 6 (Widgets, Network, WebSockets, WebEngineWidgets)
- Protocol Buffers (抖音直播 WebSocket 协议解码)
- C++20 / CMake

## 构建

### macOS

```bash
brew install qt protobuf cmake
cmake -B build -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build -j$(sysctl -n hw.ncpu)
./build/DouyinModerator
```

### Windows

参见 [WINDOWS.md](WINDOWS.md)

## 使用

1. 启动程序，点击 **登录** 扫码登录抖音
2. 输入直播间号，点击 **连接**
3. 在 **自动回复规则** 标签配置触发规则
4. 在 **设置** 标签配置定时弹幕和自动点赞

## 默认规则

| 触发条件 | 回复内容 |
|---------|---------|
| 进入房间 | 欢迎 {user} 来到直播间！[欢迎] |
| 关注主播 | 感谢 {user} 的关注！[比心] |
| 收到礼物 | 感谢 {user} 送的{gift}！[玫瑰] |
| 加入粉丝团 | 欢迎 {user} 加入粉丝团！[666] |

## 许可证

MIT
