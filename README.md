# im-control

[中文版](README_zh_CN.md)

Yet another [`im-select`](https://github.com/daipeihust/im-select) implementation for Windows.

Advantages over `im-select`:

- Works for nearly all kinds of windows, including console window and UWP windows.
- Allows for switching IM by TIP's GUID, while `im-select` only switches languages.
- Allows for setting compartments, including:
  - keyboard on/off state (e.g. Chinese/English mode for Chinese IMEs or Kana/English mode for Japanese IMEs).
  - conversion mode (e.g. alphanumeric/native mode).

## How It Works

`im-select` uses `ActivateKeyboardLayout` to switch keyboard layouts (languages), which not work for some kinds of windows, e.g. console windows and UWP windows.

On modern Windows, input methods are managed by the Text Services Framework (TSF). Each input method is represented by a Text Input Processor (TIP), which is identified by a globally unique identifier (GUID). The active TIP can be changed using the `ITfInputProcessorProfileMgr::ActivateProfile` method, which works for nearly all kinds of windows.

However, TSF requires the caller to be in the same thread as the foreground window. In order to effectively call TSF APIs, `im-control` injects a window procedure foreground window using `SetWindowsHookEx`, and then sends a custom window message to the injected window to execute TSF APIs. Other parameters are passed through shared memory to the target window.

Using this approach, `im-control` can call all TSF methods on the target window, e.g., `ITfCompartment::SetValue`, is used to set compartments.

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
im-control
im-control [LANGID-{GUID}] [-k|--keyboard <open|close>] [-c|--conversion-mode <alphamumeric|native[,...]>]
im-control -l|--list
im-control -v|--version
```

Before running, please make sure the following files are in the same directory:

- `im-control.exe`
- `im-control-injector-32.dll`
- `im-control-injector-64.dll`
- `im-control-hook-32.dll`
- `im-control-hook-64.dll`

Switch IM by `LANGID-{GUID}`:

```bash
# English (United States)
im-control 0409-{00000000-0000-0000-0000-000000000000}

# Chinese (Simplified, China) - Microsoft Pinyin
im-control 0804-{81D4E9C9-1D3B-41BC-9E6C-4B40BF79E35E}

# Weasel (RIME)
im-control 0804-{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}
```

Set keyboard on/off state (for Chinese IMEs, it sets Chinese/English mode):

```bash
im-control -k open
im-control -k close
```

Or combine them:

```bash
# Switch to Weasel and turn on Chinese input mode
im-control 0804-{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A} -k open
```

List all IMs (WIP):

```bash
im-control -l
```

Get current IM:

```bash
im-control
```

Example output:

```
0804-{81D4E9C9-1D3B-41BC-9E6C-4B40BF79E35E}
```

## History

### v0.3.0 (2025/10/14)

- Change command line syntax to be more compatible with existing tools.

### v0.2.0 (2025/10/14)

- Add implementation for getting current IM.
- Add `-version` `--version` to show version information.

### v0.1.0 (2025/10/14)

Initial release.
