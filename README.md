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

## Usage

```
im-control [LANGID-{GUID}] [-k|--keyboard <open|close>] [-c|--conversion-mode <alphamumeric|native[,...]>] [--if <LANGID-{GUID}>] [--else <LANGID-{GUID}>] [-o FILE]
im-control -l|--list
im-control -v|--version
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

### Set Keyboard On/Off State (For Chinese input methods, this sets Chinese/English mode.)

```bash
im-control -k open
im-control -k close
```

### Or combine them

```bash
# Switch to Weasel and turn on Chinese input mode
im-control 0804-{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A} -k open
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

### Get Current Input Method

```bash
im-control
```

## History

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
