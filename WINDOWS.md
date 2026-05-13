# Windows 构建指南

## 前置条件

1. **Visual Studio 2022**（安装"使用 C++ 的桌面开发"工作负载）
2. **CMake 3.19+**（VS 自带或从 cmake.org 安装）
3. **Qt 6.11**（从 https://www.qt.io/download 开源版，安装时勾选 MSVC 2022 64-bit）
4. **vcpkg**（用于安装 protobuf、zlib）

## 安装步骤

### 1. 安装 vcpkg 和依赖

```cmd
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
bootstrap-vcpkg.bat
vcpkg integrate install
vcpkg install protobuf:x64-windows zlib:x64-windows
```

### 2. 设置环境变量

```cmd
set VCPKG_ROOT=C:\vcpkg
set QT_DIR=C:\Qt\6.11.0\msvc2022_64
```

### 3. 构建

**方法 A：使用构建脚本（推荐）**

在"Developer Command Prompt for VS 2022"中运行：
```cmd
cd 项目目录
build_windows.bat
```

**方法 B：手动构建**

```cmd
cmake -B build_windows -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_PREFIX_PATH="C:\Qt\6.11.0\msvc2022_64;C:\vcpkg\installed\x64-windows"
cmake --build build_windows --config Release
```

### 4. 运行

```cmd
build_windows\Release\DouyinModerator.exe
```

## 打包发布

使用 `windeployqt` 自动收集 Qt DLL：

```cmd
C:\Qt\6.11.0\msvc2022_64\bin\windeployqt.exe build_windows\Release\DouyinModerator.exe
```

还需手动复制：
- `C:\vcpkg\installed\x64-windows\bin\libprotobuf.dll`
- `C:\vcpkg\installed\x64-windows\bin\zlib1.dll`
