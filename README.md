# im-control
Yet another im-select implementation for Windows.

## Build
```bash
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Debug
```

## Install
```bash
cmake --install build --config Debug
```

## Usage
Switch IM by GUID:
```bash
im-control -profile <GUID>
```

Set conversion mode:
```bash
im-control -convmode <alphanumeric|native>
```

Or combine them:
```bash
im-control -profile <GUID> -convmode <alphanumeric|native>
```

List all IMs:
```bash
im-control -l
```


