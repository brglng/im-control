# im-control

另一个 Windows 上的 [`im-select`](https://github.com/daipeihust/im-select) 实现。

相对于 `im-select` 的优势 :

- 支持几乎所有类型的窗口，包括控制台窗口和 UWP 窗口。
- 允许通过输入法（即 TIP, 文本输入处理器）的 GUID 切换输入法，而 `im-select` 只能切换语言。
- 允许设置输入法的区段（compartments），包括：
  - 开关状态（例如中文输入法的英文/中文模式或日语输入法的英文/假名模式）。
  - 转换模式（例如英文/本地模式）。

## 工作原理

`im-select` 使用 `ActivateKeyboardLayout` 来切换键盘布局（语言），但这对某些类型的窗口不起作用，如控制台窗口和 UWP 窗口。

在现代 Windows 上，输入法由文本服务框架（TSF）管理。每个输入法由一个文本输入处理器（TIP）表示，该处理器由一个全局唯一标识符（GUID）标识。可以使用 `ITfInputProcessorProfileMgr::ActivateProfile` 方法更改活动 TIP，该方法适用于几乎所有类型的窗口。

然而，TSF 要求调用者与目标窗口在同一线程中。为了使其工作，`im-control` 使用 `SetWindowsHookEx` 将 DLL 中的窗口过程注入目标进程，然后向被注入的窗口发送一个窗口消息以执行 TIP 切换。参数则通过共享内存传递给目标窗口。

使用这种方法，`im-control` 可以在目标窗口上调用所有 TSF 方法，例如用于设置区段的 `ITfCompartment::SetValue`。

## 编译与安装

请确保你已经安装了 CMake 和 Visual Studio。

```bash
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config RelWithDebInfo

# 将输出的二进制文件放在 `bin` 目录下
cmake --install build --prefix bin --config RelWithDebInfo
```

## 用法

在运行前，请确保以下文件在同一目录下：

- `im-control.exe`
- `im-control-injector-32.dll`
- `im-control-injector-64.dll`
- `im-control-hook-32.dll`
- `im-control-hook-64.dll`

使用语言 ID 和/或 TIP 的 GUID 切换输入法：

```bash
# 英语（美国）
im-control -langid 0x0409

# 中文（简体，中国）- 微软拼音
im-control -guidProfile {81D4E9C9-1D3B-41BC-9E6C-4B40BF79E35E}

# 小狼毫
im-control -guidProfile {A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}
```

设置键盘开关（对于中文输入法，此即中/英模式）：

```bash
im-control -keyboardOpen
im-control -keyboardClose
```

或者将它们结合起来：

```bash
# 切换到小狼毫并开启中文输入模式
im-control -langid 0x0804 -guidProfile {A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A} -keyboardOpen
```

列出所有输入法（开发中）：

```bash
im-control -l
```

获取当前输入法（开发中）：

```bash
im-control
```

## 版本历史

### v0.1.0 (2025/10/14)

第一个版本。
