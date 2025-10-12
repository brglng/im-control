# im-control
Yet another im-select implementation for Windows.

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
Switch IM by TIP's GUID:
```bash
im-control --guid-profile <GUID>
```

Set conversion mode (WIP):
```bash
im-control --conversion-mode <alphanumeric|native>
```

Or combine them (WIP):
```bash
im-control --guid-profile <GUID> --conversion-mode <alphanumeric|native>
```

List all IMs (WIP):
```bash
im-control -l
```


