# Building on Windows 11

This guide covers multiple methods to build the SDL3 + LuaJIT + Sol3 project on Windows 11.

## Prerequisites

All methods require:
- **Git**: [Download](https://git-scm.com/download/win)
- **CMake**: [Download](https://cmake.org/download/) (3.14 or newer)

---

## Option 1: vcpkg (Recommended)

[vcpkg](https://vcpkg.io/) is Microsoft's C++ package manager and provides the smoothest experience.

### Step 1: Install vcpkg

```powershell
# Clone vcpkg (recommended location: C:\vcpkg)
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# (Optional) Add to PATH for convenience
# Add C:\vcpkg to your system PATH
```

### Step 2: Install Dependencies

```powershell
cd C:\vcpkg

# Install 64-bit dependencies (recommended)
.\vcpkg install sdl3:x64-windows sdl3-ttf:x64-windows luajit:x64-windows

# Or for 32-bit
.\vcpkg install sdl3:x86-windows sdl3-ttf:x86-windows luajit:x86-windows
```

### Step 3: Build the Project

```powershell
# Clone or navigate to the project
cd path\to\cpp_libsdl3_script_luajit_sol3_embed

# Create build directory
mkdir build
cd build

# Configure with vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake

# Build
cmake --build . --config Release

# Run
.\Release\SDL3_Lua_Sol3.exe
```

### Notes
- vcpkg automatically handles DLL copying
- Use `-DVCPKG_TARGET_TRIPLET=x64-windows` to force 64-bit builds
- For Visual Studio: open the folder directly, VS will detect CMake

---

## Option 2: MSYS2/MinGW

[MSYS2](https://www.msys2.org/) provides a Unix-like environment with package management.

### Step 1: Install MSYS2

1. Download from [msys2.org](https://www.msys2.org/)
2. Run the installer
3. Open "MSYS2 MINGW64" terminal (not MSYS2 MSYS)

### Step 2: Install Dependencies

```bash
# Update package database
pacman -Syu

# Install build tools
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc mingw-w64-x86_64-ninja

# Install SDL3 and dependencies
pacman -S mingw-w64-x86_64-SDL3 mingw-w64-x86_64-SDL3_ttf

# Install LuaJIT
pacman -S mingw-w64-x86_64-luajit

# Install pkg-config (used by CMakeLists.txt)
pacman -S mingw-w64-x86_64-pkg-config
```

### Step 3: Build the Project

```bash
# Navigate to project directory
cd /c/path/to/cpp_libsdl3_script_luajit_sol3_embed

# Create build directory
mkdir build && cd build

# Configure (use Ninja for faster builds)
cmake .. -G "Ninja"

# Or use MinGW Makefiles
cmake .. -G "MinGW Makefiles"

# Build
cmake --build .

# Run
./SDL3_Lua_Sol3.exe
```

### Notes
- Always use the "MINGW64" terminal, not "MSYS2"
- Executables depend on MinGW DLLs; distribute them together
- Package availability depends on MSYS2 repository updates

---

## Option 3: Visual Studio (Manual Setup)

For full Visual Studio integration without package managers.

### Step 1: Install Visual Studio

1. Download [Visual Studio 2022](https://visualstudio.microsoft.com/) (Community is free)
2. Select "Desktop development with C++" workload
3. Ensure CMake tools are included

### Step 2: Download Dependencies Manually

#### SDL3
1. Go to [SDL Releases](https://github.com/libsdl-org/SDL/releases)
2. Download `SDL3-devel-x.x.x-VC.zip`
3. Extract to `C:\libs\SDL3`

#### SDL3_ttf
1. Go to [SDL_ttf Releases](https://github.com/libsdl-org/SDL_ttf/releases)
2. Download `SDL3_ttf-devel-x.x.x-VC.zip`
3. Extract to `C:\libs\SDL3_ttf`

#### LuaJIT
1. Option A: Build from source ([LuaJIT.org](https://luajit.org/download.html))
2. Option B: Use prebuilt binaries from vcpkg export or other sources
3. Place in `C:\libs\LuaJIT`

### Step 3: Configure CMake

The project's CMakeLists.txt uses pkg-config. For Visual Studio, you may need to:

1. Set environment variables or CMake cache variables:
```powershell
cmake .. -DSDL3_DIR=C:\libs\SDL3\cmake ^
         -DSDL3_ttf_DIR=C:\libs\SDL3_ttf\cmake ^
         -DLUAJIT_INCLUDE_DIR=C:\libs\LuaJIT\include ^
         -DLUAJIT_LIBRARY=C:\libs\LuaJIT\lib\lua51.lib
```

2. Or modify CMakeLists.txt to use `find_package()` instead of `pkg_check_modules()`

### Step 4: Build

```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

---

## Troubleshooting

### "SDL3 not found"
- Ensure SDL3 is installed via your chosen method
- For vcpkg: verify with `vcpkg list | findstr sdl3`
- For MSYS2: verify with `pacman -Qs SDL3`

### "LuaJIT not found"
- Check that pkg-config can find it: `pkg-config --cflags --libs luajit`
- For vcpkg: packages may be named differently; check `vcpkg search luajit`

### "pkg-config not found" (Visual Studio)
- Install pkg-config via chocolatey: `choco install pkgconfiglite`
- Or modify CMakeLists.txt to use `find_package()` instead

### DLL errors when running
- Copy required DLLs to the executable directory:
  - `SDL3.dll`
  - `SDL3_ttf.dll`
  - `lua51.dll` (LuaJIT)
- Or add the DLL directories to your PATH

### Sol3 download fails
- Check internet connection
- Manually clone: `git clone https://github.com/ThePhD/sol2.git external\sol2`
- Update CMakeLists.txt to use local path

### Assets not found
- Ensure `assets/` and `scripts/` directories are copied to the build output
- CMake should handle this automatically; check the build log

---

## Runtime Requirements

When distributing the application, include:
- `SDL3.dll`
- `SDL3_ttf.dll`
- `lua51.dll`
- `assets/` folder with fonts
- `scripts/` folder with Lua scripts

For MSYS2/MinGW builds, you may also need:
- `libgcc_s_seh-1.dll`
- `libstdc++-6.dll`
- `libwinpthread-1.dll`

---

## Recommended Development Setup

For the best experience on Windows:

1. **IDE**: Visual Studio Code with CMake Tools extension, or CLion
2. **Package Manager**: vcpkg integrated with your IDE
3. **Terminal**: Windows Terminal with PowerShell or MSYS2

### VS Code Setup
```json
// .vscode/settings.json
{
    "cmake.configureSettings": {
        "CMAKE_TOOLCHAIN_FILE": "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
    }
}
```

### CLion Setup
1. File → Settings → Build → CMake
2. Add to CMake options: `-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake`
