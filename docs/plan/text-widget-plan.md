# Text Rendering and Text Widget Implementation Plan

## Overview
Add text rendering and a full-featured text widget to the SDL3/LuaJIT/Sol3 project using an incremental 3-phase approach with hybrid C++/Lua architecture.

## Status

| Phase | Description | Status |
|-------|-------------|--------|
| Phase 1 | Foundation (Basic Text Rendering) | ✅ Complete |
| Phase 2 | Text Widget Primitives | ✅ Complete |
| Phase 3 | Text Widget | 🔄 In Progress |

## Files to Modify
- `src/main.cpp` - Core changes: TTF init, font management, text rendering, TextWidget class
- `CMakeLists.txt` - Add SDL3_ttf dependency and assets copy rule
- `scripts/main.lua` - Test examples

## Phase 1: Foundation (Basic Text Rendering) ✅

> **Status:** Complete. All font management and text rendering functions are implemented.

### 1.1 CMakeLists.txt Changes ✅
```cmake
pkg_check_modules(SDL3_TTF REQUIRED sdl3-ttf)
# Add to include dirs: ${SDL3_TTF_INCLUDE_DIRS}
# Add to link libs: ${SDL3_TTF_LIBRARIES}
# Add post-build copy of assets/ directory
```

### 1.2 src/main.cpp Changes ✅

**New includes:**
```cpp
#include <SDL3_ttf/SDL_ttf.h>
#include <map>
```

**New members in Application class:**
```cpp
TTF_TextEngine* textEngine = nullptr;

// Font management with size caching
struct FontEntry {
    std::string path;
    std::map<float, TTF_Font*> sizeCache;  // size -> font instance
};
std::map<int, FontEntry> fonts;  // fontId -> FontEntry
int nextFontId = 1;
int currentFontId = 0;
float currentFontSize = 16.0f;
TTF_Font* currentFont = nullptr;

TTF_Font* getOrCreateFontAtSize(int fontId, float size);  // helper
```

**Size caching**: When a font is loaded, it's cached at requested sizes. Calling `setFontSize()` or `drawText()` with a new size loads and caches that size automatically.

**In initialize():** Initialize TTF and create text engine
**In cleanup():** Destroy fonts, text engine, call TTF_Quit()

**New Lua API:**
- `loadFont(path, size)` → fontId or nil
- `setFont(fontId)` → bool
- `setFontSize(size)` → bool (changes size of current font)
- `getFontSize()` → size
- `closeFont(fontId)`
- `measureText(text)` → {width, height}
- `getFontHeight()` → height
- `drawText(text, x, y, r, g, b, a)`
- `drawText(text, x, y, size, r, g, b, a)` (overload with per-call size)

---

## Phase 2: Text Widget Primitives ✅

> **Status:** Complete. All event callbacks, text input control, clipboard, drawing helpers, and text measurement functions are implemented.

### New Event Callbacks
- `onTextInput(text)` - SDL_EVENT_TEXT_INPUT
- `onKeyUp(key)` - SDL_EVENT_KEY_UP
- `onMouseUp(x, y, button)` - SDL_EVENT_MOUSE_BUTTON_UP
- `onMouseWheel(x, y, scrollX, scrollY)` - SDL_EVENT_MOUSE_WHEEL
- `onMouseMove(x, y)` - SDL_EVENT_MOUSE_MOTION
- `onTouchDown(fingerId, x, y, pressure)` - SDL_EVENT_FINGER_DOWN
- `onTouchUp(fingerId, x, y)` - SDL_EVENT_FINGER_UP
- `onTouchMove(fingerId, x, y, dx, dy)` - SDL_EVENT_FINGER_MOTION

### New Lua API
**Text input control:**
- `startTextInput()`, `stopTextInput()`, `isTextInputActive()`
- `setTextInputArea(x, y, w, h, cursorOffset)`

**Clipboard:**
- `getClipboardText()`, `setClipboardText(text)`, `hasClipboardText()`

**Keyboard:**
- `getKeyModifiers()` → {shift, ctrl, alt, gui}

**Drawing helpers:**
- `drawLine(x1, y1, x2, y2, r, g, b, a)`
- `drawRectOutline(x, y, w, h, r, g, b, a)`

**Text measurement:**
- `measureTextToOffset(text, byteOffset)` → width
- `getOffsetFromX(text, x)` → byteOffset

---

## Phase 3: Text Widget

### TextWidget C++ Class
Full-featured widget supporting:
- Single-line or multi-line mode (configurable)
- Text cursor with blinking
- Selection (click-drag, shift+arrow, Ctrl+A)
- Editing (insert, delete, backspace)
- Clipboard (Ctrl+C/V/X)
- Navigation (arrows, Home, End, Up/Down for multiline)

### Scrolling Features
**Input methods:**
- Arrow keys (Up/Down scroll in read-only, move cursor in editable)
- Page Up/Page Down keys
- Mouse wheel (vertical scrolling)
- Touch swipe/fling (with momentum/inertia)
- Horizontal: Shift+wheel or horizontal swipe

**Scrollbars:**
- Configurable visibility: `always`, `auto` (show when content overflows), `never`
- Configurable interactivity: `draggable` (click and drag thumb), `clickable` (click track to jump), `none`
- Both horizontal and vertical scrollbars
- Scrollbar styling: width, colors (track, thumb, thumb-hover)

**Config options:**
```lua
createTextWidget({
    -- ... existing options ...
    scrollbars = {
        vertical = "auto",     -- "always", "auto", "never"
        horizontal = "auto",
        draggable = true,      -- can drag scrollbar thumb
        clickTrack = true,     -- click track to jump
        width = 12,            -- scrollbar width in pixels
    },
    scrollMomentum = true,     -- touch fling momentum
})
```

### Key Components
```cpp
class TextWidget {
    // State: text, cursorPos, selectionStart/End, scrollX/Y
    // Config: x, y, width, height, multiline, editable, colors
    // Methods: setText, getText, handleKeyDown, handleTextInput,
    //          handleMouseDown/Up/Move, update, render
};
```

### Lua API
```lua
createTextWidget({x, y, width, height, multiline, editable}) → TextWidget

widget:setText(text)
widget:getText()
widget:setPosition(x, y)
widget:setSize(w, h)
widget:setMultiline(bool)
widget:setFocus(bool)
widget:hasFocus()
widget:update(dt)
widget:render()
```

### Event Routing
Modify handleEvents() to route keyboard/mouse/text events to focused TextWidget before Lua callbacks.

---

## Implementation Order

| Step | Task |
|------|------|
| 1.1 | CMakeLists.txt: add SDL3_ttf pkg-config | ✅ |
| 1.2 | Add TTF_Init, TTF_CreateRendererTextEngine | ✅ |
| 1.3 | Add loadFont, setFont, closeFont | ✅ |
| 1.4 | Add measureText, getFontHeight | ✅ |
| 1.5 | Add drawText | ✅ |
| 1.6 | Test Phase 1 in Lua | ✅ |
| 2.1 | Add onTextInput, onKeyUp, onMouseUp events | ✅ |
| 2.2 | Add startTextInput/stopTextInput | ✅ |
| 2.3 | Add clipboard functions | ✅ |
| 2.4 | Add getKeyModifiers | ✅ |
| 2.5 | Add drawLine, drawRectOutline | ✅ |
| 2.6 | Add measureTextToOffset, getOffsetFromX | ✅ |
| 2.7 | Test Phase 2 in Lua (test_phase2.lua) | ✅ |
| 3.1 | Implement TextWidget class | ✅ |
| 3.2 | Add TextWidget Lua bindings | ✅ |
| 3.3 | Add event routing to widgets | ✅ |
| 3.4 | Test full widget functionality | ✅ |
| 3.5 | Add Page Up/Page Down keys | |
| 3.6 | Add mouse wheel scrolling for widgets | |
| 3.7 | Add scrollbar UI (vertical/horizontal) | |
| 3.8 | Add scrollbar interactivity (drag, click track) | |
| 3.9 | Add touch swipe/fling with momentum | |
| 3.10 | Improve widget system architecture | |

---

## Notes
- Font file already exists: `assets/DejaVuSans.ttf`
- SDL3_ttf already available via pkg-config
- Uses modern TTF_Text API with TTF_CreateRendererTextEngine for GPU-accelerated rendering
- UTF-8 aware cursor navigation needed (use SDL3 UTF-8 utilities)
