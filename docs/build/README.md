# Build Documentation

Platform-specific build instructions for the SDL3 + LuaJIT + Sol3 project.

## Quick Links

| Platform | Documentation |
|----------|---------------|
| Windows 11 | [WINDOWS.md](WINDOWS.md) |
| Linux | [LINUX.md](LINUX.md) |
| macOS | [MACOS.md](MACOS.md) |

## Quick Start

### Linux (Ubuntu/Debian)
```bash
sudo apt install cmake build-essential libsdl3-dev libsdl3-ttf-dev libluajit-5.1-dev pkg-config
mkdir build && cd build && cmake .. && cmake --build .
./SDL3_Lua_Sol3
```

### macOS
```bash
brew install cmake sdl3 sdl3_ttf luajit pkg-config
mkdir build && cd build && cmake .. && cmake --build .
./SDL3_Lua_Sol3
```

### Windows (vcpkg)
```powershell
vcpkg install sdl3:x64-windows sdl3-ttf:x64-windows luajit:x64-windows
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release
.\Release\SDL3_Lua_Sol3.exe
```

## Dependencies

| Dependency | Version | Purpose |
|------------|---------|---------|
| SDL3 | 3.x | Window, rendering, input |
| SDL3_ttf | 3.x | TrueType font rendering |
| LuaJIT | 2.1+ | Lua scripting engine |
| Sol3 | 3.x | C++/Lua bindings (auto-fetched) |
| CMake | 3.14+ | Build system |

## Common Issues

See platform-specific guides for detailed troubleshooting. Common issues:

- **SDL3 not available**: SDL3 is new; may need to build from source
- **pkg-config not found**: Install via package manager or use CMake's find_package
- **Sol3 download fails**: Clone manually to `external/sol2`
- **Assets not found**: Run from build directory or copy assets alongside executable

## Build Types

```bash
# Debug (with symbols, no optimization)
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release (optimized)
cmake .. -DCMAKE_BUILD_TYPE=Release

# RelWithDebInfo (optimized with debug symbols)
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
```
