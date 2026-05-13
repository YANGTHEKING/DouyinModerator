@echo off
chcp 65001 >nul
echo ============================================
echo   抖音直播房管工具 - Windows 构建脚本
echo ============================================
echo.

:: 检查依赖
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 未找到 CMake，请先安装: https://cmake.org/download/
    echo        或者通过 Visual Studio Installer 安装
    pause
    exit /b 1
)

where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo [提示] 未检测到 MSVC 编译器
    echo        请从 "开始菜单" 运行 "Developer Command Prompt for VS 2022"
    echo        或者运行 Visual Studio 的 vcvarsall.bat:
    echo        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
    pause
    exit /b 1
)

:: 配置 Qt 路径 - 按实际情况修改
set QT_DIR=
if exist "C:\Qt\6.11.0\msvc2022_64" (
    set QT_DIR=C:\Qt\6.11.0\msvc2022_64
) else if exist "C:\Qt\6.10.0\msvc2022_64" (
    set QT_DIR=C:\Qt\6.10.0\msvc2022_64
) else if exist "C:\Qt\6.9.0\msvc2022_64" (
    set QT_DIR=C:\Qt\6.9.0\msvc2022_64
) else if exist "D:\Qt\6.11.0\msvc2022_64" (
    set QT_DIR=D:\Qt\6.11.0\msvc2022_64
) else if defined QT_DIR (
    rem 使用环境变量
) else (
    echo [错误] 未找到 Qt 安装路径
    echo        请修改此脚本中的 QT_DIR 变量，指向你的 Qt MSVC 安装目录
    echo        例如: set QT_DIR=C:\Qt\6.11.0\msvc2022_64
    pause
    exit /b 1
)

echo [信息] Qt 路径: %QT_DIR%

:: 配置 protobuf 路径 - 如果通过 vcpkg 安装
set PROTOBUF_DIR=
if defined VCPKG_ROOT (
    set PROTOBUF_DIR=%VCPKG_ROOT%\installed\x64-windows
    echo [信息] vcpkg 路径: %PROTOBUF_DIR%
)

:: 构建 CMake 前缀路径
set CMAKE_PREFIX_PATH=%QT_DIR%
if defined PROTOBUF_DIR (
    set CMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH%;%PROTOBUF_DIR%
)

:: 清理旧构建
if exist build_windows (
    echo [信息] 清理旧的构建目录...
    rmdir /s /q build_windows
)

:: 配置
echo.
echo [步骤 1/3] CMake 配置...
cmake -B build_windows -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    2>&1

if %errorlevel% neq 0 (
    echo [错误] CMake 配置失败
    pause
    exit /b 1
)

:: 编译
echo.
echo [步骤 2/3] 编译中...
cmake --build build_windows --config Release -j %NUMBER_OF_PROCESSORS% 2>&1

if %errorlevel% neq 0 (
    echo [错误] 编译失败
    pause
    exit /b 1
)

:: 部署 Qt 依赖
echo.
echo [步骤 3/3] 收集 Qt 运行时依赖...
if exist "%QT_DIR%\bin\windeployqt.exe" (
    "%QT_DIR%\bin\windeployqt.exe" ^
        --qmldir "%QT_DIR%\qml" ^
        --no-translations ^
        build_windows\Release\DouyinModerator.exe
    echo [信息] Qt 依赖收集完成
) else (
    echo [警告] 未找到 windeployqt.exe，请手动运行:
    echo   %QT_DIR%\bin\windeployqt.exe build_windows\Release\DouyinModerator.exe
)

echo.
echo ============================================
echo   构建完成!
echo ============================================
echo.
echo   可执行文件: build_windows\Release\DouyinModerator.exe
echo.
echo   首次运行前确保已安装:
echo   - Visual C++ Redistributable (通常已随 VS 安装)
echo.
echo   打包发布时，需要包含以下 DLL:
echo   - Qt6Core.dll, Qt6Gui.dll, Qt6Widgets.dll
echo   - Qt6Network.dll, Qt6WebSockets.dll
echo   - Qt6WebEngineCore.dll, Qt6WebEngineWidgets.dll
echo   - protobuf 和 zlib DLL (从 vcpkg 安装目录复制)
echo   - platforms/ 目录 (qwindows.dll)
echo   - Qt WebEngine 运行时文件
echo ============================================
echo.
pause
