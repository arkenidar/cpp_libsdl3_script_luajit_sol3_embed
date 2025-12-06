# Building on macOS

This guide covers building the SDL3 + LuaJIT + Sol3 project on macOS (Intel and Apple Silicon).

## Prerequisites

- macOS 10.15 (Catalina) or newer
- Xcode Command Line Tools
- Homebrew package manager

### Install Xcode Command Line Tools

```bash
xcode-select --install
```

### Install Homebrew

If not already installed:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

---

## Installation

### Install Dependencies via Homebrew

```bash
# Install build tools
brew install cmake pkg-config

# Install SDL3 and SDL3_ttf
brew install sdl3 sdl3_ttf

# Install LuaJIT
brew install luajit
```

### Verify Installation

```bash
# Check versions
cmake --version
pkg-config --version

# Check SDL3
pkg-config --cflags --libs sdl3

# Check LuaJIT
pkg-config --cflags --libs luajit
```

---

## Building

### Standard Build

```bash
# Navigate to project
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

### Release Build (Optimized)

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Using Ninja (Faster Builds)

```bash
brew install ninja
cmake .. -G Ninja
ninja
```

---

## Apple Silicon (M1/M2/M3) Notes

Homebrew on Apple Silicon installs to `/opt/homebrew` instead of `/usr/local`.

### If pkg-config can't find packages

```bash
# Add Homebrew to PKG_CONFIG_PATH
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:$PKG_CONFIG_PATH"

# Then build
cmake ..
cmake --build .
```

### For Universal Binaries (Intel + ARM)

```bash
cmake .. -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build .
```

---

## Creating an App Bundle

For distribution, create a proper macOS app bundle:

### Basic Structure

```
SDL3_Lua_Sol3.app/
├── Contents/
│   ├── Info.plist
│   ├── MacOS/
│   │   └── SDL3_Lua_Sol3
│   └── Resources/
│       ├── assets/
│       │   └── DejaVuSans.ttf
│       └── scripts/
│           └── main.lua
```

### Create Info.plist

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>SDL3_Lua_Sol3</string>
    <key>CFBundleIdentifier</key>
    <string>com.yourname.sdl3luademo</string>
    <key>CFBundleName</key>
    <string>SDL3 Lua Demo</string>
    <key>CFBundleVersion</key>
    <string>1.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
```

### Bundle Creation Script

```bash
#!/bin/bash
APP_NAME="SDL3_Lua_Sol3"
BUNDLE_DIR="${APP_NAME}.app/Contents"

mkdir -p "${BUNDLE_DIR}/MacOS"
mkdir -p "${BUNDLE_DIR}/Resources/assets"
mkdir -p "${BUNDLE_DIR}/Resources/scripts"

cp build/${APP_NAME} "${BUNDLE_DIR}/MacOS/"
cp -r assets/* "${BUNDLE_DIR}/Resources/assets/"
cp -r scripts/* "${BUNDLE_DIR}/Resources/scripts/"
cp Info.plist "${BUNDLE_DIR}/"

echo "Bundle created: ${APP_NAME}.app"
```

---

## Troubleshooting

### "SDL3 not found"

```bash
# Reinstall SDL3
brew reinstall sdl3

# Check Homebrew prefix
brew --prefix sdl3

# Set PKG_CONFIG_PATH if needed
export PKG_CONFIG_PATH="$(brew --prefix)/lib/pkgconfig:$PKG_CONFIG_PATH"
```

### "luajit not found"

```bash
# Check if installed
brew list luajit

# Reinstall if needed
brew reinstall luajit

# Link if unlinked
brew link luajit
```

### Library not loaded errors

```bash
# Check library dependencies
otool -L SDL3_Lua_Sol3

# Fix rpath if needed
install_name_tool -add_rpath /opt/homebrew/lib SDL3_Lua_Sol3
```

### Homebrew packages outdated

```bash
# Update Homebrew
brew update
brew upgrade
```

### Code signing issues (Gatekeeper)

```bash
# For development, allow unsigned apps
sudo spctl --master-disable

# Or sign the app
codesign --force --deep --sign - SDL3_Lua_Sol3.app
```

---

## IDE Integration

### Xcode

1. Generate Xcode project:
   ```bash
   cmake .. -G Xcode
   open SDL3_Lua_Sol3.xcodeproj
   ```

2. Build with Cmd+B, Run with Cmd+R

### Visual Studio Code

1. Install extensions: "C/C++" and "CMake Tools"
2. Open the project folder
3. Select kit when prompted (Clang)
4. Build with CMake Tools

### CLion

1. Open the project folder
2. CLion auto-detects CMake
3. Build with Cmd+F9

---

## Performance Tips

### Enable Link-Time Optimization

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

### Use System LuaJIT for best performance

LuaJIT from Homebrew is pre-optimized for macOS.

### Retina Display

The demo automatically handles high-DPI displays via SDL3.

---

## Distribution Checklist

When distributing the app:

1. Create proper app bundle (see above)
2. Include all required assets
3. Sign the application: `codesign --sign "Developer ID" SDL3_Lua_Sol3.app`
4. Notarize for Gatekeeper (if distributing outside App Store)
5. Create DMG for distribution: `hdiutil create -volname "SDL3 Lua Demo" -srcfolder SDL3_Lua_Sol3.app -ov -format UDZO SDL3_Lua_Demo.dmg`
