# im-control 使用指南

im-control 通过 TSF (Text Services Framework) compartments 控制 Windows 输入法。
支持切换输入法、控制键盘开关、设置中英文模式、查询当前状态。

## 安装

编译后的 `im-control.exe` 放于 PATH 中或固定路径即可。

## 其他命令

```bash
im-control -V            # 打印版本号
im-control --version
im-control -h            # 打印用法
im-control --help
```

## 列出可用输入法

```bash
im-control -l
```

输出格式：`LANGID-{GUID} 语言: 输入法名称`

```
0804-{E7EA138E-69F8-11D7-A6EA-00065B844310} zh-CN: 微软拼音
0804-{E7EA138F-69F8-11D7-A6EA-00065B844311} zh-CN: 小狼毫
0409-{00000000-0000-0000-0000-000000000000} en-US: ENG
```

## 基本命令

### 切换输入法

```bash
# 切换到指定输入法
im-control 0804-{GUID}
```

### 控制键盘开关

```bash
im-control -k open     # 打开键盘
im-control -k close    # 关闭键盘（部分 IME 会变灰）
```

### 控制中英文模式

```bash
im-control -c alphanumeric   # 英文模式（半角英数）
im-control -c native         # 中文模式
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

## 组合使用

```bash
# 切换输入法 + 打开键盘 + 英文模式
im-control 0804-{GUID} -k open -c alphanumeric

# 切换输入法 + 打开键盘 + 中文模式
im-control 0804-{GUID} -k open -c native
```

## 条件切换

```bash
# 当前是 RIME 则切英文输入法，否则切回 RIME
im-control --if 0804-{RIME-GUID} 0409-{ENG-GUID} --else 0804-{RIME-GUID}
```

## -k close vs -c alphanumeric

| 命令 | 键盘状态 | IME 托盘图标 | 效果 |
|------|----------|-------------|------|
| `-k close` | 关闭 | 变灰（禁用） | 直接输入英文 |
| `-c alphanumeric` | 打开 | 显示 EN | IME 工作在英文模式 |

对于需要在应用不同模式下保持 IME 激活但切换模式的场景，使用 `-c alphanumeric`；
对于完全不需要 IME 的场景，使用 `-k close`。

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
