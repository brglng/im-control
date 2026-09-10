# im-control

[中文版](README_zh_CN.md)

Yet another [`im-select`](https://github.com/daipeihust/im-select) implementation for Windows.

Advantages over `im-select`:

- Works for nearly all kinds of windows, including console window and UWP windows.
- Allows for switching IM by TIP's GUID, while `im-select` only switches languages.
- Allows for setting compartments, including:
  - keyboard on/off state (e.g. Chinese/English mode for Chinese IMEs or Kana/English mode for Japanese IMEs).
  - conversion mode (e.g. alphanumeric/native mode).
- Console window won't flash when running from GUI.
- Other features, including conditional switching, etc.

## How It Works

`im-select` uses `ActivateKeyboardLayout` to switch keyboard layouts (languages), which not work for some kinds of windows, e.g. console windows and UWP windows.

On modern Windows, input methods are managed by the Text Services Framework (TSF). Each input method is represented by a Text Input Processor (TIP), which is identified by a globally unique identifier (GUID). The active TIP can be changed using the `ITfInputProcessorProfileMgr::ActivateProfile` method, which works for nearly all kinds of windows.

However, TSF requires the caller to be in the same thread as the foreground window. In order to effectively call TSF APIs, `im-control` injects a window procedure foreground window using `SetWindowsHookEx`, and then sends a custom window message to the injected window to execute TSF APIs. Other parameters are passed through shared memory to the target window.

Using this approach, `im-control` can call all TSF APIs on the target window, e.g., `ITfCompartment::SetValue`, is used to set compartments.

## Build and Install

Make sure you have CMake and Visual Studio installed.

```bash
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config RelWithDebInfo

# place the output binaries in `bin` directory
cmake --install build --prefix bin --config RelWithDebInfo
```

After building, you can place `im-control.exe` in PATH or a fixed path. The helper DLLs listed below must remain in the same directory as the executable.

## Usage

```
im-control [LANGID-{GUID}] [-k|--keyboard <open|close>] [-c|--conversion-mode <alphanumeric|native[,...]>] [-g|--get-keyboard] [--if <LANGID-{GUID}>] [--else <LANGID-{GUID}>] [-o FILE]
im-control -l|--list
im-control -V|--version
im-control -h|--help
```

Before running, please make sure the following files are in the same directory:

- `im-control.exe`
- `im-control-injector-32.dll`
- `im-control-injector-64.dll`
- `im-control-hook-32.dll`
- `im-control-hook-64.dll`

### Switch Input Method by `LANGID-{GUID}`

```bash
# English (United States)
im-control 0409-{00000000-0000-0000-0000-000000000000}

# Chinese (Simplified, China) - Microsoft Pinyin
im-control 0804-{81D4E9C9-1D3B-41BC-9E6C-4B40BF79E35E}

# Weasel (RIME)
im-control 0804-{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}
```

The output is the previous IM's `LANGID-{GUID}`.

### Set Keyboard On/Off State

```bash
im-control -k open
im-control -k close
```

### Set Conversion Mode

```bash
im-control -c alphanumeric   # English mode (half-width alphanumeric)
im-control -c native         # Chinese mode
```

### Or Combine the Above

```bash
# Switch to Weasel and turn on Chinese input mode
im-control 0804-{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A} -k open

# Switch to Weasel, turn on the keyboard and English mode
im-control 0804-{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A} -k open -c alphanumeric

# Switch to Weasel, turn on the keyboard and Chinese mode
im-control 0804-{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A} -k open -c native
```

### Conditional Switching

```bash
# If current input method is en-US keyboard, switch to Chinese Pinyin, else switch to Weasel
im-control 0804-{81D4E9C9-1D3B-41BC-9E6C-4B40BF79E35E} --if 0409-{00000000-0000-0000-0000-000000000000} --else 0804-{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}
```

### List All Input Methods

```bash
im-control -l
```

Output format: `LANGID-{GUID} language: input method name`

```
0804-{E7EA138E-69F8-11D7-A6EA-00065B844310} zh-CN: Microsoft Pinyin
0804-{E7EA138F-69F8-11D7-A6EA-00065B844311} zh-CN: Weasel
0409-{00000000-0000-0000-0000-000000000000} en-US: ENG
```

### Get Current Input Method

```bash
im-control
```

### Get Keyboard State

```bash
im-control -g
# Output: open native          — keyboard on, Chinese mode
# Output: open alphanumeric    — keyboard on, English mode
# Output: close                — keyboard off
```

Write the output to a file:

```bash
im-control -g -o /tmp/ime-status
```

### Other Commands

```bash
im-control -V            # print version number
im-control --version
im-control -h            # print usage
im-control --help
```

## `-k close` vs `-c alphanumeric`

| Command | Keyboard state | IME tray icon | Effect |
|---------|----------------|---------------|--------|
| `-k close` | Off | Grayed out (disabled) | Types English directly |
| `-c alphanumeric` | On | Shows EN | IME works in English mode |

Use `-c alphanumeric` when you want to keep the IME active but switch modes in different app contexts; use `-k close` when you don't need the IME at all.

## Windows 11 Compatibility Notes

im-control works on both Windows 10 and Windows 11, but the TSF framework behaves differently on Windows 11, and these differences are already handled in the code:

1. **ThreadMgr instance**: On Windows 11, `CoCreateInstance(CLSID_TF_ThreadMgr)` returns a new instance rather than a per-thread singleton. im-control uses `TF_GetThreadMgr` (exported by msctf.dll) to get the correct singleton.
2. **TfClientId validation**: On Windows 11, `SetValue` only triggers `OnChange` for an activated client (non-zero `TfClientId`). im-control calls `ITfThreadMgr::Activate` to obtain a valid ID.

### Working with Weasel (RIME)

The CONVERSION and OPENCLOSE compartments of WeaselTSF operate independently:

- **Switching Chinese/English**: only write CONVERSION (`-c native` / `-c alphanumeric`), do not write OPENCLOSE.
- **Enabling/disabling the IME**: only write OPENCLOSE (`-k open` / `-k close`), do not write CONVERSION.

Avoid writing both compartments at the same time, as it may trigger the blind toggle of the OPENCLOSE handler and flip the Chinese/English state.

If you need to switch the input method and the Chinese/English mode at the same time, it is recommended to call twice:
```bash
im-control 0804-{GUID}          # switch the input method first
im-control -c alphanumeric      # then switch the mode
```

### Related Documentation

- [Weasel compartment external control fix analysis](https://github.com/VimWei/weasel/blob/im-control/docs/compartment-external-control-fix.md) — the complete root-cause analysis and fix plan

## History

### v0.5.1 (2026/09/10)

- Add `-g|--get-keyboard` option to query keyboard state.
- Fix Windows 11 compatibility: use the `TF_GetThreadMgr` singleton instead of `CoCreateInstance`, and use a valid `TfClientId` to trigger `OnChange`.
- Fix OPENCLOSE behavior when working with Weasel (RIME): skip `SetValue` when the value is unchanged, and reopen OPENCLOSE on Windows 10 when gvim disables the RIME keyboard.
- Fix potential permanent hang and singleton deadlock.
- Validate VARIANT type before reading/writing the conversion compartment.
- Improve error handling for keyboard state queries.
- Remove debug logging and restore log file mode.

### v0.5.0 (2025/10/24)

- Implement `-l|--list` option.

### v0.4.0 (2025/10/18)

- Compile as Windows GUI application to avoid console window flash, but still supports to attach to console when run from console.
- Use Event instead of waiting for process exit to reduce wait time.
- Add `-o|--output FILE` option to write output to file.
- Add `--if` and `--else` options for conditional switching.
- Unify GUID format to upper case letters.
- Refactor and improve injector code.
- Improve logging.
- Other bug fixes.

### v0.3.0 (2025/10/14)

- Change command line syntax to be more compatible with existing tools.

### v0.2.0 (2025/10/14)

- Add implementation for getting current IM.
- Add `-version` `--version` to show version information.

### v0.1.0 (2025/10/14)

Initial release.
