# im-control
Yet another [`im-select`](https://github.com/daipeihust/im-select) implementation for Windows.

Advantages over `im-select`:
- Works for nearly all kinds of windows, including console window and UWP windows.
- Allows for switching IM by TIP's GUID, while `im-select` only switches languages.

## Build
```bash
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config RelWithDebInfo
```

## Install
```bash
cmake --install build --config RelWithDebInfo
```

## Usage
Switch IM by langid and/or TIP's GUID:
```bash
# English (United States)
im-control -langid 0x0409

# Chinese (Simplified, China) - Microsoft Pinyin
im-control -guidProfile {81D4E9C9-1D3B-41BC-9E6C-4B40BF79E35E}

# Weasel
im-control -guidProfile {A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}
```

Set conversion mode (WIP):
```bash
im-control -conversionMode <ALPHANUMERIC|NATIVE>
```

Or combine them (WIP):
```bash
im-control -langid <LANGID> -guidProfile <GUID> -conversionMode <ALPHANUMERIC|NATIVE>
```

List all IMs (WIP):
```bash
im-control -l
```

Get current IM (WIP):
```bash
im-control
```
