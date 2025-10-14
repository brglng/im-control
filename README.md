# im-control

[中文版](README_zh_CN.md)

Yet another [`im-select`](https://github.com/daipeihust/im-select) implementation for Windows.

Advantages over `im-select`:

- Works for nearly all kinds of windows, including console window and UWP windows.
- Allows for switching IM by TIP's GUID, while `im-select` only switches languages.
- Allows for setting compartments, including:
  - keyboard on/off state (e.g. English/Chinese mode for Chinese IMEs or English/Kana mode for Japanese IMEs).
  - conversion mode (e.g. alphanumeric/native mode).

## How It Works

`im-select` uses `ActivateKeyboardLayout` to switch keyboard layouts (languages), which not work for some kinds of windows, e.g. console windows and UWP windows.

On modern Windows, input methods are managed by the Text Services Framework (TSF). Each input method is represented by a Text Input Processor (TIP), which is identified by a globally unique identifier (GUID). The active TIP can be changed using the `ITfInputProcessorProfileMgr::ActivateProfile` method, which works for nearly all kinds of windows.

However, TSF requires the caller to be in the same thread as the target window. To make it work, `im-control` injects a DLL into the target process by injecting a window procedure using `SetWindowsHookEx`, and then sends a window message to the injected window to execute the profile switching code. The parameters are passed through shared memory to the target window.

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

Before running, please make sure the following files are in the same directory:

- `im-control.exe`
- `im-control-injector-32.dll`
- `im-control-injector-64.dll`
- `im-control-hook-32.dll`
- `im-control-hook-64.dll`

Switch IM by langid and/or TIP's GUID:

```bash
# English (United States)
im-control -langid 0x0409

# Chinese (Simplified, China) - Microsoft Pinyin
im-control -guidProfile {81D4E9C9-1D3B-41BC-9E6C-4B40BF79E35E}

# Weasel (RIME)
im-control -guidProfile {A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}
```

Set keyboard on/off state (for Chinese IMEs, it's English/Chinese mode):

```bash
im-control -keyboardOpen
im-control -keyboardClose
```

Or combine them

```bash
# Switch to Weasel and turn on Chinese input mode
im-control -langid 0x0804 -guidProfile {A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A} -keyboardOpen
```

List all IMs (WIP)

```bash
im-control -l
```

Get current IM (WIP)

```bash
im-control
```

## History

### v0.1.0 (2025/10/14)

Initial release.
