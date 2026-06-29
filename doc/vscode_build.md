# C3DSlicer 在 VS Code 下的构建、运行与调试

## 1. 目标

这份文档针对当前 Windows 开发环境，目标是：

- 不再依赖 Visual Studio IDE 图形界面
- 仍然继续使用本机已有的 MSVC 编译器、Windows SDK、CMake
- 在 VS Code 里完成 `配置 -> 编译 -> 运行 -> 调试`

说明：

- 这个项目当前已经有一套稳定的 Windows 构建脚本：`project_build.bat`
- 仓库里的 `CMakePresets.json` 主要偏向 Ninja/跨平台场景
- 你平时 Visual Studio 下使用的依赖路径，和当前 Windows preset 不是完全同一套

所以在 VS Code 下，推荐分成两条路线：

1. 兼容性最高：继续调用现有 `bat` 脚本，只是把入口换成 VS Code
2. 更原生：使用 VS Code 的 CMake Tools，但建议通过 `CMakeUserPresets.json` 单独维护本机配置

如果你只是想先稳定迁移到 VS Code，优先走路线 1。

---

## 2. 当前仓库里已经确认的关键信息

### 2.1 现有 Windows 构建脚本

- 入口脚本：[project_build.bat](C:/WORK/C3DSlicer/project_build.bat)
- 旧脚本：[build_release_vs2022.bat](C:/WORK/C3DSlicer/build_release_vs2022.bat)

其中 `project_build.bat` 更贴近你当前习惯的 Visual Studio 工作流。

### 2.2 当前可用依赖目录

本机已经存在：

- `C:\WORK\C3DSlicer\dep_Release\usr\local`

这说明你当前 Windows 构建更适合继续走 `dep_Release` 这条依赖链。

### 2.3 当前可执行文件位置

仓库里已经存在的 Release 可执行文件是：

- [CrealityPrint.exe](C:/WORK/C3DSlicer/build_Release/src/Release/CrealityPrint.exe)

所以 VS Code 里“运行/调试”的目标程序，当前最靠谱的路径就是这个。

---

## 3. 前置准备

### 3.1 推荐安装的 VS Code 扩展

至少装这几个：

- `C/C++`，Microsoft
- `CMake Tools`，Microsoft
- `C/C++ Extension Pack`，可选

### 3.2 推荐的打开方式

不要直接从普通 PowerShell 启动 VS Code。

推荐从下面任意一种终端里进入仓库再执行 `code .`：

1. `x64 Native Tools Command Prompt for VS 2022`
2. `Developer PowerShell for VS 2022`

这样 VS Code 继承到的环境里会带上：

- `cl.exe`
- `link.exe`
- Windows SDK
- `cmake`

如果你直接从普通终端打开 VS Code，经常会遇到：

- 找不到 MSVC 编译器
- CMake 配置成功但链接失败
- Debugger 或 SDK 环境不完整

---

## 4. 路线 1：最稳的 VS Code 工作流

这条路线本质上是：

- 编辑器换成 VS Code
- 构建仍然复用项目已有的 `bat` 脚本

优点：

- 和你现在 Visual Studio 下的行为最接近
- 最不容易踩依赖路径的坑
- 迁移成本最低

### 4.1 在 VS Code 终端里构建 Release

在仓库根目录执行：

```powershell
.\project_build.bat Project Release
```

如果只想重新生成工程：

```powershell
.\project_build.bat Generate Release
```

如果只想 build 已生成工程：

```powershell
.\project_build.bat Build Release
```

### 4.2 在 VS Code 终端里运行

```powershell
.\build_Release\src\Release\CrealityPrint.exe
```

### 4.3 这条路线适合什么场景

适合：

- 你先想把“只用 VS Code 也能干活”跑通
- 你不想一上来就改 CMake preset
- 你需要最大程度保持和既有构建结果一致

不适合：

- 你想完全依赖 CMake Tools 的按钮式体验
- 你想切 Debug/Release 都通过 preset 统一管理

---

## 5. 路线 2：VS Code + CMake Tools 原生工作流

这条路线更像“脱离 Visual Studio IDE，但保留 MSVC 编译工具链”。

### 5.1 为什么不能直接无脑用当前 `CMakePresets.json`

当前仓库的 Windows preset：

- `x64-debug`
- `x64-release`

虽然能被 VS Code 识别，但它们默认是 `Ninja`，而且没有带上你当前最常用的 Windows 依赖路径：

- `C:\WORK\C3DSlicer\dep_Release\usr\local`

而你现有稳定构建脚本实际走的是：

- Visual Studio 2022 generator
- `dep_Release\usr\local`
- `build_Release`

所以如果直接点 VS Code 的 configure/build，很可能出现：

- 找不到依赖
- 链接失败
- 生成目录和你原有目录体系不一致

### 5.2 推荐做法

不要直接改仓库里的 `CMakePresets.json`。

推荐在仓库根目录新建你自己的：

- `CMakeUserPresets.json`

这不会影响别人，也更适合本机环境。

### 5.3 推荐的本机 `CMakeUserPresets.json`

下面这份更接近你现在 Visual Studio 的工作方式：

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "vscode-release-local",
      "displayName": "VS Code Release Local",
      "generator": "Visual Studio 17 2022",
      "architecture": "x64",
      "binaryDir": "${sourceDir}/build_Release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "BBL_RELEASE_TO_PUBLIC": "1",
        "GENERATE_ORCA_HEADER": "ON",
        "CMAKE_INSTALL_PREFIX": "./CrealityPrint",
        "CMAKE_PREFIX_PATH": "C:/WORK/C3DSlicer/dep_Release/usr/local"
      }
    },
    {
      "name": "vscode-debug-local",
      "displayName": "VS Code Debug Local",
      "generator": "Visual Studio 17 2022",
      "architecture": "x64",
      "binaryDir": "${sourceDir}/build_Debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "BBL_RELEASE_TO_PUBLIC": "1",
        "GENERATE_ORCA_HEADER": "ON",
        "CMAKE_INSTALL_PREFIX": "./CrealityPrint",
        "CMAKE_PREFIX_PATH": "C:/WORK/C3DSlicer/dep_Release/usr/local"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "vscode-release-local",
      "configurePreset": "vscode-release-local",
      "configuration": "Release"
    },
    {
      "name": "vscode-debug-local",
      "configurePreset": "vscode-debug-local",
      "configuration": "Debug"
    }
  ]
}
```

说明：

- `binaryDir` 直接复用你现在熟悉的 `build_Release` / `build_Debug`
- `CMAKE_PREFIX_PATH` 显式指向本机已有依赖目录
- generator 用 `Visual Studio 17 2022`，更接近你原来的行为

### 5.4 在 VS Code 中如何使用

1. 打开命令面板
2. 执行 `CMake: Select Configure Preset`
3. 选择 `vscode-release-local`
4. 执行 `CMake: Configure`
5. 执行 `CMake: Build`

如果要切 Debug：

1. `CMake: Select Configure Preset`
2. 选择 `vscode-debug-local`
3. `CMake: Configure`
4. `CMake: Build`

---

## 6. 在 VS Code 里运行和调试

### 6.1 最直接的运行方式

不配 `launch.json` 也可以，直接在终端执行：

```powershell
.\build_Release\src\Release\CrealityPrint.exe
```

### 6.2 推荐的 `launch.json`

如果你希望在 VS Code 里直接按 `F5` 调试，可以在本机 `.vscode/launch.json` 写：

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Run CrealityPrint Release",
      "type": "cppvsdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build_Release/src/Release/CrealityPrint.exe",
      "args": [],
      "stopAtEntry": false,
      "cwd": "${workspaceFolder}",
      "environment": [],
      "console": "integratedTerminal"
    }
  ]
}
```

如果后面你要调 Debug，可把 `program` 改到 Debug 版本路径。

说明：

- `.vscode/` 已经在 `.gitignore` 里忽略了
- 所以本机调试配置可以放心放这里，不会污染仓库

---

## 7. 我对当前项目的建议

结合当前仓库状态，建议分两步走。

### 第一步

先只做“VS Code 作为入口”，不要急着完全切到 CMake Tools：

- 从 `Developer PowerShell for VS 2022` 打开 `code .`
- 在 VS Code 终端执行：

```powershell
.\project_build.bat Project Release
.\build_Release\src\Release\CrealityPrint.exe
```

这样最快，风险最低。

### 第二步

等第一步稳定后，再补本机：

- `CMakeUserPresets.json`
- `.vscode/launch.json`
- 可选 `.vscode/tasks.json`

这样你就可以在 VS Code 里做到：

- 选择 preset
- 点击 configure/build
- F5 调试

---

## 8. 常见问题

### 8.1 为什么 VS Code 里 configure 成功，但 build 失败

通常是下面几类原因：

- VS Code 不是从 VS 开发者终端启动的
- `CMAKE_PREFIX_PATH` 没指到本机依赖目录
- 选了仓库默认的 Windows preset，但它并不是你现在这套依赖路径

### 8.2 为什么我更推荐 `CMakeUserPresets.json`

因为当前这个项目：

- 已有共享的 `CMakePresets.json`
- 但你的本机依赖路径明显是定制的

把这些本机差异放进 `CMakeUserPresets.json`，比直接改共享 preset 更稳。

### 8.3 VS Code 能不能完全替代 Visual Studio IDE

可以。

前提是你本机已经有：

- Visual Studio 2022 或 Build Tools
- MSVC 工具链
- Windows SDK
- CMake

本质上你不是不用 Visual Studio 编译器，而是不再用 Visual Studio 的图形 IDE。

---

## 9. 下一步我可以继续帮你做什么

如果你要，我下一步可以直接继续做这些本机配置文件：

1. 生成一份适合你这台机器的 `CMakeUserPresets.json`
2. 生成 `.vscode/launch.json`
3. 生成 `.vscode/tasks.json`
4. 顺手帮你把 `Build / Run / Debug` 三条链路在 VS Code 里跑通
