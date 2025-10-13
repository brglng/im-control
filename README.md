# im-control

Yet another [`im-select`](https://github.com/daipeihust/im-select) implementation for Windows.

另一个 Windows 上的 [`im-select`](https://github.com/daipeihust/im-select) 实现。

Advantages over `im-select`:

- Works for nearly all kinds of windows, including console window and UWP windows.
- Allows for switching IM by TIP's GUID, while `im-select` only switches languages.
- Allows for setting compartments, including:
  - keyboard on/off state (e.g. English/Chinese mode for Chinese IMEs or English/Kana mode for Japanese IMEs).
  - conversion mode (e.g. alphanumeric/native mode).

相对于 `im-select` 的优势 :

- 支持几乎所有类型的窗口，包括控制台窗口和 UWP 窗口。
- 允许通过输入法（即 TIP, 文本输入处理器）的 GUID 切换输入法，而 `im-select` 只能切换语言。
- 允许设置输入法的区段（compartments），包括：
  - 开关状态（例如中文输入法的英文/中文模式或日语输入法的英文/假名模式）。
  - 转换模式（例如英文/本地模式）。

## How It Works 工作原理

`im-select` uses `ActivateKeyboardLayout` to switch keyboard layouts (languages), which not work for some kinds of windows, e.g. console windows and UWP windows.

`im-select` 使用 `ActivateKeyboardLayout` 来切换键盘布局（语言），但这对某些类型的窗口不起作用，如控制台窗口和 UWP 窗口。

On modern Windows, input methods are managed by the Text Services Framework (TSF). Each input method is represented by a Text Input Processor (TIP), which is identified by a globally unique identifier (GUID). The active TIP can be changed using the `ITfInputProcessorProfileMgr::ActivateProfile` method, which works for nearly all kinds of windows.

在现代 Windows 上，输入法由文本服务框架（TSF）管理。每个输入法由一个文本输入处理器（TIP）表示，该处理器由一个全局唯一标识符（GUID）标识。可以使用 `ITfInputProcessorProfileMgr::ActivateProfile` 方法更改活动 TIP，该方法适用于几乎所有类型的窗口。

However, TSF requires the caller to be in the same thread as the target window. To make it work, `im-control` injects a DLL into the target process by injecting a window procedure using `SetWindowsHookEx`, and then sends a window message to the injected window to execute the profile switching code. The parameters are passed through shared memory to the target window.

然而，TSF 要求调用者与目标窗口在同一线程中。为了使其工作，`im-control` 通过使用 `SetWindowsHookEx` 将 DLL 中的窗口过程注入目标进程，然后向被注入的窗口发送一个窗口消息以执行 TIP 切换。参数则通过共享内存传递给目标窗口。

Using this approach, `im-control` can call all TSF methods on the target window, e.g., `ITfCompartment::SetValue`, is used to set compartments.

使用这种方法，`im-control` 可以在目标窗口上调用所有 TSF 方法，例如用于设置区段的 `ITfCompartment::SetValue`。

## Build 编译

Make sure you have CMake and Visual Studio installed.

请确保你已经安装了 CMake 和 Visual Studio。

```bash
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config RelWithDebInfo
```

## Install 安装

```bash
cmake --install build --config RelWithDebInfo
```

## Usage 用法

Before running, please make sure the following files are in the same directory:

在运行前，请确保以下文件在同一目录下：

- `im-control.exe`
- `im-control-injector-32.dll`
- `im-control-injector-64.dll`
- `im-control-hook-32.dll`
- `im-control-hook-64.dll`

Switch IM by langid and/or TIP's GUID:

使用语言 ID 和/或 TIP 的 GUID 切换输入法：

```bash
# English (United States)
# 英语（美国）
im-control -langid 0x0409

# Chinese (Simplified, China) - Microsoft Pinyin
# 中文（简体，中国）- 微软拼音
im-control -guidProfile {81D4E9C9-1D3B-41BC-9E6C-4B40BF79E35E}

# Weasel
# 小狼毫
im-control -guidProfile {A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}
```

Set keyboard on/off state:

设置键盘开关：

```bash
im-control -keyboardOpen
im-control -keyboardClose
```

Or combine them:

或者将它们结合起来：

```bash
# Switch to Weasel and turn on Chinese input mode
# 切换到小狼毫并开启中文输入模式
im-control -langid 0x0804 -guidProfile {A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A} -keyboardOpen
```

List all IMs (WIP):

列出所有输入法（开发中）：

```bash
im-control -l
```

Get current IM (WIP):

获取当前输入法（开发中）：

```bash
im-control
```

## History 历史

### 2025-10-14: v0.1.0

Initial release.

第一个版本。
