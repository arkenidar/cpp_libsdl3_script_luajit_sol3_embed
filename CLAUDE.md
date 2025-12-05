# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Standard CMake build process
mkdir build && cd build
cmake ..
cmake --build .

# Run the application (from build directory)
./SDL3_Lua_Sol3
```

The project uses CMake with Ninja generator and requires SDL3, LuaJIT, and Sol3 dependencies.

## Architecture Overview

This is an SDL3 application with embedded LuaJIT scripting using Sol3 bindings. The architecture follows a single-class design:

### Core Components

- **Application class** (`src/main.cpp:6-189`): Main application controller that manages SDL3 window/renderer, Sol3 Lua state, and the game loop
- **Lua API bindings** (`src/main.cpp:59-98`): C++ functions exposed to Lua for window management, drawing, and application control
- **Event handling** (`src/main.cpp:111-131`): SDL3 event processing with callbacks to Lua functions
- **Game loop** (`src/main.cpp:162-176`): Standard update/render loop with delta timing

### Lua Integration

The application automatically loads `scripts/main.lua` and expects these Lua callback functions:
- `update(deltaTime)` - Called every frame for game logic
- `render()` - Called every frame for drawing
- `onKeyDown(keyName)` - Called on key press events

### Exposed Lua API

Window management: `setWindowTitle()`, `getWindowSize()`, `setBackgroundColor()`  
Drawing: `drawRect(x, y, w, h, r, g, b, a)`  
Application control: `quit()`, `print()`

## Key Files

- `src/main.cpp` - Single-file application containing all C++ logic
- `scripts/main.lua` - Main Lua script with demo animations and event handling
- `CMakeLists.txt` - Build configuration with Sol3 FetchContent integration

## Development Notes

Scripts are automatically copied to the build directory during CMake configuration. The application loads scripts relative to the executable location, so run from the build directory.

Sol3 is fetched automatically via CMake FetchContent from GitHub. LuaJIT and SDL3 must be system-installed with pkg-config support.