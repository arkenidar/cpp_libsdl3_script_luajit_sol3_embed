# Building on Linux

This guide covers building the SDL3 + LuaJIT + Sol3 project on various Linux distributions.

## Prerequisites

All distributions require:
- CMake 3.14 or newer
- C++17 compatible compiler (GCC 7+ or Clang 5+)
- pkg-config
- Git

---

## Ubuntu / Debian / Debian 13 (Trixie)

### Install Dependencies

```bash
# Update package list
sudo apt update

# Install build tools
sudo apt install cmake build-essential pkg-config git

# Install SDL3 and SDL3_ttf
sudo apt install libsdl3-dev libsdl3-ttf-dev

# Install LuaJIT
sudo apt install libluajit-5.1-dev
```

**Note for Debian 13 (Trixie):** SDL3 packages are available in the official repositories. Trixie includes `libsdl3-dev` and `libsdl3-ttf-dev` natively.

**Note for older Debian/Ubuntu:** SDL3 may not be in default repos; see below for building from source.

### If SDL3 is not in repositories (older distributions)

SDL3 is relatively new. If not available:

```bash
# Build SDL3 from source
git clone https://github.com/libsdl-org/SDL.git
cd SDL
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
sudo cmake --install .

# Build SDL3_ttf from source
cd ../..
git clone https://github.com/libsdl-org/SDL_ttf.git
cd SDL_ttf
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
sudo cmake --install .

# Update library cache
sudo ldconfig
```

### Build the Project

```bash
# Clone or navigate to the project
cd cpp_libsdl3_script_luajit_sol3_embed

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .

# Run
./SDL3_Lua_Sol3
```

---

## Fedora / RHEL / CentOS

### Install Dependencies

```bash
# Install build tools
sudo dnf install cmake gcc-c++ pkgconfig git

# Install SDL3 and SDL3_ttf
sudo dnf install SDL3-devel SDL3_ttf-devel

# Install LuaJIT
sudo dnf install luajit-devel
```

### Build the Project

```bash
mkdir build && cd build
cmake ..
cmake --build .
./SDL3_Lua_Sol3
```

---

## Arch Linux / Manjaro

### Install Dependencies

```bash
# Install from official repos
sudo pacman -S cmake base-devel pkgconf git

# SDL3 (check AUR if not in main repos)
sudo pacman -S sdl3 sdl3_ttf

# Or from AUR
yay -S sdl3-git sdl3_ttf-git

# Install LuaJIT
sudo pacman -S luajit
```

### Build the Project

```bash
mkdir build && cd build
cmake ..
cmake --build .
./SDL3_Lua_Sol3
```

---

## openSUSE

### Install Dependencies

```bash
# Install build tools
sudo zypper install cmake gcc-c++ pkg-config git

# Install SDL3 (may need to build from source)
sudo zypper install libSDL3-devel libSDL3_ttf-devel

# Install LuaJIT
sudo zypper install luajit-devel
```

---

## Generic / Build from Source

If your distribution doesn't have SDL3 packages:

### 1. Install Build Dependencies

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake pkg-config git \
    libfreetype-dev libharfbuzz-dev

# Fedora
sudo dnf install gcc-c++ cmake pkgconfig git \
    freetype-devel harfbuzz-devel
```

### 2. Build SDL3

```bash
git clone https://github.com/libsdl-org/SDL.git
cd SDL
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build . -j$(nproc)
sudo cmake --install .
cd ../..
```

### 3. Build SDL3_ttf

```bash
git clone https://github.com/libsdl-org/SDL_ttf.git
cd SDL_ttf
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build . -j$(nproc)
sudo cmake --install .
cd ../..
```

### 4. Install LuaJIT

```bash
git clone https://luajit.org/git/luajit.git
cd luajit
make -j$(nproc)
sudo make install
cd ..
```

### 5. Update Library Path

```bash
sudo ldconfig
```

### 6. Build the Project

```bash
cd cpp_libsdl3_script_luajit_sol3_embed
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/usr/local
cmake --build .
./SDL3_Lua_Sol3
```

---

## Build Options

### Debug Build

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

### Release Build (Optimized)

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Using Ninja (Faster Builds)

```bash
# Install ninja
sudo apt install ninja-build  # Ubuntu/Debian
sudo dnf install ninja-build  # Fedora

# Build with Ninja
cmake .. -G Ninja
ninja
```

### Specify Install Prefix

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build .
cmake --install .
```

---

## Troubleshooting

### "SDL3 not found"

```bash
# Check if pkg-config can find SDL3
pkg-config --cflags --libs sdl3

# If not, set PKG_CONFIG_PATH
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
```

### "LuaJIT not found"

```bash
# Check pkg-config
pkg-config --cflags --libs luajit

# Package might be named differently
pkg-config --list-all | grep -i lua
```

### "error while loading shared libraries"

```bash
# Update library cache
sudo ldconfig

# Or set LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

### Sol3 Download Fails

```bash
# Clone manually
git clone https://github.com/ThePhD/sol2.git external/sol2

# Then build
mkdir build && cd build
cmake ..
cmake --build .
```

### Permission Denied

```bash
# Make executable
chmod +x SDL3_Lua_Sol3

# Run
./SDL3_Lua_Sol3
```

---

## IDE Integration

### Visual Studio Code

1. Install extensions: "C/C++" and "CMake Tools"
2. Open the project folder
3. CMake Tools will auto-detect and configure

### CLion

1. Open the project folder (contains CMakeLists.txt)
2. CLion will auto-detect CMake project
3. Build with Ctrl+F9 or the hammer icon

### Qt Creator

1. File → Open File or Project
2. Select CMakeLists.txt
3. Configure and build

---

## Running from Different Directory

The application expects `scripts/` and `assets/` relative to the executable:

```bash
# Option 1: Run from build directory
cd build
./SDL3_Lua_Sol3

# Option 2: Copy assets to run from anywhere
cp -r scripts assets /path/to/destination/
cd /path/to/destination
/path/to/build/SDL3_Lua_Sol3
```
