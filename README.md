# Directions

## Linux

1. Ensure FUSE is installed (it should be by default)
2. `cmake . && cmake --build .`
3. Run with `main /mount/path -s`

Developed/tested with:

- Ubuntu 24.04
- GCC 13.3
- CMake 3.28.0

## Windows

1. Install [WinFSP](https://winfsp.dev/rel)
2. Add WinFSP's `bin` folder to your path
3. Set the `WINFSP_LIB_PATH` cache variable to the `winfsp-*.lib` corresponding to your platform
4. `cmake . && cmake --build .`
5. Run with `main /mount/path -s`

Developed/tested with:

- Windows 11
- WinFSP 2.0
- Visual Studio 2022 Community (MSVC version 19.38.33134)
- CMake 3.31.0
