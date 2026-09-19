# im-control

另一个 Windows 上的 [`im-select`](https://github.com/daipeihust/im-select) 实现。

相对于 `im-select` 的优势 :

- 支持几乎所有类型的窗口，包括控制台窗口和 UWP 窗口。
- 允许通过输入法（即 TIP，文本输入处理器）的 GUID 切换输入法，而 `im-select` 只能切换语言。
- 允许设置输入法的区段（compartments），包括：
  - 开关状态（例如中文输入法的中文/英文模式或日语输入法的假名/英文模式）。
  - 转换模式（例如英文/本地模式）。
- 从 GUI 运行时，不会出现控制台窗口。
- 其他功能，包括条件切换等。

## 工作原理

`im-select` 使用 `ActivateKeyboardLayout` 来切换键盘布局（语言），但这对某些类型的窗口不起作用，如控制台窗口和 UWP 窗口。

在现代 Windows 上，输入法由文本服务框架（TSF）管理。每个输入法由一个文本输入处理器（TIP）表示，并由一个全局唯一标识符（GUID）标识。可以使用 `ITfInputProcessorProfileMgr::ActivateProfile` 方法更改活动 TIP，该方法适用于几乎所有类型的窗口。

然而，TSF 要求调用者与前台窗口在同一线程中。为了有效地调用 TSF API，`im-control` 使用 `SetWindowsHookEx` 将 DLL 中的窗口过程注入前台窗口，然后向被注入的窗口发送一个自定义的窗口消息以调用 TSF 的 API。其他参数则通过共享内存传递给目标窗口。

使用这种方法，`im-control` 可以在目标窗口上调用所有 TSF API，例如用于设置区段的 `ITfCompartment::SetValue`。

## 编译与安装

请确保你已经安装了 CMake 和 Visual Studio。

```bash
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config RelWithDebInfo

# 将输出的二进制文件放在 `bin` 目录下
cmake --install build --prefix bin --config RelWithDebInfo
```

编译完成后会生成以下两个可执行文件：

- `im-control.exe`：Console 子系统版本，使用继承的标准输入输出，适合 Shell 和脚本调用。
- `im-controlw.exe`：GUI 子系统版本，启动时创建隐藏的私有 Console，仅在产生控制台输出时显示。

下面列出的辅助 DLL 必须与可执行文件放在同一目录。

## 用法

```
im-control [LANGID-{GUID}] [-k|--keyboard <open|close>] [-c|--conversion-mode <alphanumeric|native[,...]>] [-g|--get-keyboard] [--if <LANGID-{GUID}>] [--else <LANGID-{GUID}>] [-o FILE]
im-control -l|--list
im-control -V|--version
im-control -h|--help
```

在运行前，请确保以下文件在同一目录下：

- `im-control.exe` 或 `im-controlw.exe`
- `im-control-injector-32.dll`
- `im-control-injector-64.dll`
- `im-control-hook-32.dll`
- `im-control-hook-64.dll`

### 使用 `LANGID-{GUID}` 切换输入法

```bash
# 英语（美国）
im-control 0409-{00000000-0000-0000-0000-000000000000}

# 中文（简体，中国）- 微软拼音
im-control 0804-{81D4E9C9-1D3B-41BC-9E6C-4B40BF79E35E}

# 小狼毫
im-control 0804-{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}
```

输出为之前输入法的 `LANGID-{GUID}`。

### 设置键盘开关

```bash
im-control -k open
im-control -k close
```

### 设置转换模式

```bash
im-control -c alphanumeric   # 英文模式（半角英数）
im-control -c native         # 中文模式
```

### 或者将以上组合起来

```bash
# 切换到小狼毫并开启中文输入模式
im-control 0804-{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A} -k open

# 切换到小狼毫、打开键盘并切换为英文模式
im-control 0804-{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A} -k open -c alphanumeric

# 切换到小狼毫、打开键盘并切换为中文模式
im-control 0804-{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A} -k open -c native
```

### 条件切换

```bash
# 如果当前输入法是英语键盘，则切换到微软拼音，否则切换到小狼毫
im-control 0804-{81D4E9C9-1D3B-41BC-9E6C-4B40BF79E35E} --if 0409-{00000000-0000-0000-0000-000000000000} --else 0804-{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}
```

### 列出所有输入法

```bash
im-control -l
```

输出格式：`LANGID-{GUID} 语言: 输入法名称`

```
0804-{E7EA138E-69F8-11D7-A6EA-00065B844310} zh-CN: 微软拼音
0804-{E7EA138F-69F8-11D7-A6EA-00065B844311} zh-CN: 小狼毫
0409-{00000000-0000-0000-0000-000000000000} en-US: ENG
```

### 获取当前输入法

```bash
im-control
```

### 查询当前状态

```bash
im-control -g
# 输出：open native           — 键盘打开，中文模式
# 输出：open alphanumeric     — 键盘打开，英文模式
# 输出：close                 — 键盘关闭
```

输出到文件：

```bash
im-control -g -o /tmp/ime-status
```

### 其他命令

```bash
im-control -V            # 打印版本号
im-control --version
im-control -h            # 打印用法
im-control --help
```

## `-k close` 与 `-c alphanumeric` 的区别

| 命令 | 键盘状态 | IME 托盘图标 | 效果 |
|------|----------|-------------|------|
| `-k close` | 关闭 | 变灰（禁用） | 直接输入英文 |
| `-c alphanumeric` | 打开 | 显示 EN | IME 工作在英文模式 |

对于需要在应用不同模式下保持 IME 激活但切换模式的场景，使用 `-c alphanumeric`；对于完全不需要 IME 的场景，使用 `-k close`。

## Windows 11 兼容性注意事项

im-control 在 Windows 10 和 Windows 11 下均可用，但 Windows 11 的 TSF 框架行为有以下差异，已在代码中处理：

1. **ThreadMgr 实例**：Windows 11 的 `CoCreateInstance(CLSID_TF_ThreadMgr)` 返回新实例而非 per-thread 单例。im-control 使用 `TF_GetThreadMgr`（msctf.dll 导出）获取正确的单例。
2. **TfClientId 验证**：Windows 11 仅为已激活客户端（非零 `TfClientId`）的 `SetValue` 触发 `OnChange`。im-control 调用 `ITfThreadMgr::Activate` 获取有效 ID。

### 与 Weasel (RIME) 配合使用

WeaselTSF 的 CONVERSION 和 OPENCLOSE 两个 compartment 独立运作：

- **切换中英文**：只写 CONVERSION（`-c native` / `-c alphanumeric`），不写 OPENCLOSE
- **启用/禁用 IME**：只写 OPENCLOSE（`-k open` / `-k close`），不写 CONVERSION

避免同时写两个 compartment，否则可能触发 OPENCLOSE handler 的 blind toggle，导致中英状态反转。

如需同时切换输入法和中英文模式，建议分两次调用：
```bash
im-control 0804-{GUID}          # 先切换输入法
im-control -c alphanumeric      # 再切换模式
```

### 相关文档

- [Weasel compartment 外部控制修复分析](https://github.com/VimWei/weasel/blob/im-control/docs/compartment-external-control-fix.md) — 完整的根因分析和修复方案

## 版本历史

### v0.5.1 (2026/09/10)

- 增加 `-g|--get-keyboard` 参数以查询键盘状态。
- 修复 Windows 11 兼容性：使用 `TF_GetThreadMgr` 单例代替 `CoCreateInstance`，并使用有效的 `TfClientId` 触发 `OnChange`。
- 修复与 Weasel (RIME) 配合使用时的 OPENCLOSE 行为：值未变化时跳过 `SetValue`，并在 Windows 10 上 gvim 禁用 RIME 键盘时重新打开 OPENCLOSE。
- 修复可能的永久挂起和单例死锁。
- 在读写转换区段前校验 VARIANT 类型。
- 改进键盘状态查询的错误处理。
- 移除调试日志并恢复日志文件模式。

### v0.5.0 (2025/10/24)

- 实现 `-l|--list` 参数以列出所有输入法。

### v0.4.0 (2025/10/18)

- 编译为 Win32 GUI 应用以避免弹出控制台窗口，但从命令行运行时仍支持附到控制台。
- 使用 Event 代替等待进程退出以减少等待时间。
- 增加 `-o|--output FILE` 参数以将输出写入文件。
- 增加 `--if` 和 `--else` 参数以支持条件切换。
- 将 GUID 输出格式统一为大写字母。
- 重构并改进注入器代码。
- 改进日志记录。
- 其他 bug 修复。

### v0.3.0 (2025/10/14)

- 改变命令行语法以更兼容现有工具。

### v0.2.0 (2025/10/14)

- 实现获取当前输入法功能。
- 增加 `-version` `--version` 参数以显示版本信息。

### v0.1.0 (2025/10/14)

第一个版本。
