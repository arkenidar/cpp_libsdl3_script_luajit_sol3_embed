# SDL3 + LuaJIT + Sol3 Integration

A shareable codebase demonstrating integration of CMake, C++, libSDL3, Sol3, and LuaJIT for embedded scripting.

## Features

- **SDL3**: Modern graphics and window management
- **LuaJIT**: High-performance Lua scripting engine
- **Sol3**: Modern C++ Lua binding library
- **CMake**: Cross-platform build system
- **Lua Scripting**: Full game loop control from Lua (update, render, events)

## Prerequisites

### Ubuntu/Debian
```bash
sudo apt-get install cmake build-essential libsdl3-dev libluajit-5.1-dev pkg-config
```

### Fedora/RHEL
```bash
sudo dnf install cmake gcc-c++ SDL3-devel luajit-devel pkgconfig
```

### macOS
```bash
brew install cmake sdl3 luajit pkg-config
```

### Windows
- Install [CMake](https://cmake.org/download/)
- Install [Visual Studio](https://visualstudio.microsoft.com/) with C++ support
- Install [vcpkg](https://vcpkg.io/) and install dependencies:
  ```powershell
  vcpkg install sdl3 luajit
  ```

## Building

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .

# Run
./SDL3_Lua_Sol3
```

## Project Structure

```
.
├── CMakeLists.txt          # Build configuration
├── src/
│   └── main.cpp            # Main C++ application
├── scripts/
│   └── main.lua            # Main Lua script
└── README.md               # This file
```

## Lua API

The following functions are exposed to Lua:

### Application Control
- `quit()` - Exit the application
- `print(message)` - Print to console with [Lua] prefix

### Window Management
- `setWindowTitle(title)` - Set window title
- `getWindowSize()` - Returns table with `width` and `height`
- `setBackgroundColor(r, g, b, a)` - Set clear color (0.0-1.0)

### Drawing
- `drawRect(x, y, w, h, r, g, b, a)` - Draw filled rectangle

### Callbacks (implement in Lua)
- `update(deltaTime)` - Called every frame with delta time in seconds
- `render()` - Called every frame for drawing
- `onKeyDown(keyName)` - Called when key is pressed

## Example Lua Script

```lua
print("Hello from Lua!")

function update(deltaTime)
    -- Update game logic
end

function render()
    local winSize = getWindowSize()
    drawRect(100, 100, 200, 150, 1.0, 0.5, 0.0, 1.0)
end

function onKeyDown(key)
    if key == "Escape" then
        quit()
    end
end
```

## Customization

### Extending the Lua API

Edit `setupLuaBindings()` in [src/main.cpp](src/main.cpp) to add more functions:

```cpp
lua["myFunction"] = [this](int param) {
    // Your C++ code
};
```

### Adding More Scripts

You can load additional Lua files from your main script:

```lua
dofile("scripts/helper.lua")
```

## Troubleshooting

### SDL3 not found
- Ensure SDL3 is installed and CMake can find it
- Set `CMAKE_PREFIX_PATH` if needed: `cmake -DCMAKE_PREFIX_PATH=/path/to/sdl3 ..`

### LuaJIT not found
- Ensure `pkg-config` can find luajit: `pkg-config --cflags --libs luajit`
- Install `libluajit-5.1-dev` or equivalent for your platform

### Sol3 download fails
- Check internet connection
- Manually clone Sol3: `git clone https://github.com/ThePhD/sol2.git external/sol2`
- Modify CMakeLists.txt to use local path instead of FetchContent

## License

This is a template project - feel free to use it however you like!

## Resources

- [SDL3 Documentation](https://wiki.libsdl.org/SDL3/)
- [Sol3 Documentation](https://sol2.readthedocs.io/)
- [LuaJIT](https://luajit.org/)
- [Lua Reference](https://www.lua.org/manual/5.1/)
