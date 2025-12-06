#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <sol/sol.hpp>
#include <iostream>
#include <string>
#include <map>

class Application {
private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool running = true;
    sol::state lua;

    int windowWidth = 800;
    int windowHeight = 600;
    SDL_FColor bgColor = {0.1f, 0.1f, 0.15f, 1.0f};

    // TTF text rendering
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

    // Helper to get or create a font at a specific size
    TTF_Font* getOrCreateFontAtSize(int fontId, float size) {
        auto it = fonts.find(fontId);
        if (it == fonts.end()) return nullptr;

        auto& entry = it->second;
        auto sizeIt = entry.sizeCache.find(size);
        if (sizeIt != entry.sizeCache.end()) {
            return sizeIt->second;
        }

        // Load font at new size
        TTF_Font* font = TTF_OpenFont(entry.path.c_str(), size);
        if (font) {
            entry.sizeCache[size] = font;
        }
        return font;
    }

public:
    Application() {
        // Initialize Lua with standard libraries
        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::string);

        // Expose SDL and application functions to Lua
        setupLuaBindings();
    }

    ~Application() {
        cleanup();
    }

    bool initialize() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
            return false;
        }

        window = SDL_CreateWindow(
            "SDL3 + LuaJIT + Sol3",
            windowWidth, windowHeight,
            SDL_WINDOW_RESIZABLE
        );

        if (!window) {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
            return false;
        }

        renderer = SDL_CreateRenderer(window, nullptr);
        if (!renderer) {
            std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
            return false;
        }

        // Initialize SDL_ttf
        if (!TTF_Init()) {
            std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
            return false;
        }

        // Create text engine for GPU-accelerated text rendering
        textEngine = TTF_CreateRendererTextEngine(renderer);
        if (!textEngine) {
            std::cerr << "TTF_CreateRendererTextEngine failed: " << SDL_GetError() << std::endl;
            return false;
        }

        std::cout << "SDL3 initialized successfully" << std::endl;
        std::cout << "LuaJIT version: " << LUA_VERSION << std::endl;

        return true;
    }

    void setupLuaBindings() {
        // Expose quit function
        lua["quit"] = [this]() { running = false; };

        // Expose window functions
        lua["setWindowTitle"] = [this](const std::string& title) {
            if (window) {
                SDL_SetWindowTitle(window, title.c_str());
            }
        };

        lua["setBackgroundColor"] = sol::overload(
            [this](float r, float g, float b) {
                bgColor = {r, g, b, 1.0f};
            },
            [this](float r, float g, float b, float a) {
                bgColor = {r, g, b, a};
            }
        );

        lua["getWindowSize"] = [this]() -> sol::table {
            sol::table size = lua.create_table();
            size["width"] = windowWidth;
            size["height"] = windowHeight;
            return size;
        };

        // Expose drawing functions
        lua["drawRect"] = [this](float x, float y, float w, float h, float r, float g, float b, float a = 1.0f) {
            if (renderer) {
                SDL_SetRenderDrawColor(renderer,
                    static_cast<Uint8>(r * 255),
                    static_cast<Uint8>(g * 255),
                    static_cast<Uint8>(b * 255),
                    static_cast<Uint8>(a * 255));
                SDL_FRect rect = {x, y, w, h};
                SDL_RenderFillRect(renderer, &rect);
            }
        };

        // Expose print function
        lua["print"] = [](const std::string& msg) {
            std::cout << "[Lua] " << msg << std::endl;
        };

        // Font management functions
        lua["loadFont"] = [this](const std::string& path, float size) -> sol::object {
            TTF_Font* font = TTF_OpenFont(path.c_str(), size);
            if (!font) {
                std::cerr << "Failed to load font: " << path << " - " << SDL_GetError() << std::endl;
                return sol::nil;
            }

            int fontId = nextFontId++;
            fonts[fontId] = FontEntry{path, {{size, font}}};

            // If no current font, set this as current
            if (currentFontId == 0) {
                currentFontId = fontId;
                currentFontSize = size;
                currentFont = font;
            }

            return sol::make_object(lua, fontId);
        };

        lua["setFont"] = [this](int fontId) -> bool {
            TTF_Font* font = getOrCreateFontAtSize(fontId, currentFontSize);
            if (!font) return false;

            currentFontId = fontId;
            currentFont = font;
            return true;
        };

        lua["setFontSize"] = [this](float size) -> bool {
            if (currentFontId == 0) return false;

            TTF_Font* font = getOrCreateFontAtSize(currentFontId, size);
            if (!font) return false;

            currentFontSize = size;
            currentFont = font;
            return true;
        };

        lua["getFontSize"] = [this]() -> float {
            return currentFontSize;
        };

        lua["closeFont"] = [this](int fontId) {
            auto it = fonts.find(fontId);
            if (it == fonts.end()) return;

            // Close all cached sizes
            for (auto& [size, font] : it->second.sizeCache) {
                TTF_CloseFont(font);
            }
            fonts.erase(it);

            // Clear current font if it was the one we closed
            if (currentFontId == fontId) {
                currentFontId = 0;
                currentFont = nullptr;
            }
        };

        // Text measurement functions
        lua["measureText"] = [this](const std::string& text) -> sol::table {
            sol::table result = lua.create_table();
            result["width"] = 0;
            result["height"] = 0;

            if (!currentFont) return result;

            int w = 0, h = 0;
            if (TTF_GetStringSize(currentFont, text.c_str(), text.length(), &w, &h)) {
                result["width"] = w;
                result["height"] = h;
            }
            return result;
        };

        lua["getFontHeight"] = [this]() -> int {
            if (!currentFont) return 0;
            return TTF_GetFontHeight(currentFont);
        };

        // Text rendering function
        lua["drawText"] = sol::overload(
            // drawText(text, x, y, r, g, b, a)
            [this](const std::string& text, float x, float y, float r, float g, float b, float a) {
                if (!currentFont || !textEngine || !renderer) return;

                TTF_Text* ttfText = TTF_CreateText(textEngine, currentFont, text.c_str(), text.length());
                if (!ttfText) return;

                SDL_Color color = {
                    static_cast<Uint8>(r * 255),
                    static_cast<Uint8>(g * 255),
                    static_cast<Uint8>(b * 255),
                    static_cast<Uint8>(a * 255)
                };
                TTF_SetTextColor(ttfText, color.r, color.g, color.b, color.a);
                TTF_DrawRendererText(ttfText, x, y);
                TTF_DestroyText(ttfText);
            },
            // drawText(text, x, y, r, g, b) - default alpha 1.0
            [this](const std::string& text, float x, float y, float r, float g, float b) {
                if (!currentFont || !textEngine || !renderer) return;

                TTF_Text* ttfText = TTF_CreateText(textEngine, currentFont, text.c_str(), text.length());
                if (!ttfText) return;

                SDL_Color color = {
                    static_cast<Uint8>(r * 255),
                    static_cast<Uint8>(g * 255),
                    static_cast<Uint8>(b * 255),
                    255
                };
                TTF_SetTextColor(ttfText, color.r, color.g, color.b, color.a);
                TTF_DrawRendererText(ttfText, x, y);
                TTF_DestroyText(ttfText);
            },
            // drawText(text, x, y, size, r, g, b, a) - with per-call size
            [this](const std::string& text, float x, float y, float size, float r, float g, float b, float a) {
                if (currentFontId == 0 || !textEngine || !renderer) return;

                TTF_Font* font = getOrCreateFontAtSize(currentFontId, size);
                if (!font) return;

                TTF_Text* ttfText = TTF_CreateText(textEngine, font, text.c_str(), text.length());
                if (!ttfText) return;

                SDL_Color color = {
                    static_cast<Uint8>(r * 255),
                    static_cast<Uint8>(g * 255),
                    static_cast<Uint8>(b * 255),
                    static_cast<Uint8>(a * 255)
                };
                TTF_SetTextColor(ttfText, color.r, color.g, color.b, color.a);
                TTF_DrawRendererText(ttfText, x, y);
                TTF_DestroyText(ttfText);
            }
        );

        // Text input control functions
        lua["startTextInput"] = [this]() {
            if (window) {
                SDL_StartTextInput(window);
            }
        };

        lua["stopTextInput"] = [this]() {
            if (window) {
                SDL_StopTextInput(window);
            }
        };

        lua["isTextInputActive"] = [this]() -> bool {
            if (window) {
                return SDL_TextInputActive(window);
            }
            return false;
        };

        lua["setTextInputArea"] = [this](float x, float y, float w, float h, int cursorOffset) {
            if (window) {
                SDL_Rect rect = {
                    static_cast<int>(x),
                    static_cast<int>(y),
                    static_cast<int>(w),
                    static_cast<int>(h)
                };
                SDL_SetTextInputArea(window, &rect, cursorOffset);
            }
        };

        // Clipboard functions
        lua["getClipboardText"] = []() -> std::string {
            char* text = SDL_GetClipboardText();
            if (text) {
                std::string result(text);
                SDL_free(text);
                return result;
            }
            return "";
        };

        lua["setClipboardText"] = [](const std::string& text) -> bool {
            return SDL_SetClipboardText(text.c_str());
        };

        lua["hasClipboardText"] = []() -> bool {
            return SDL_HasClipboardText();
        };

        // Keyboard modifier state
        lua["getKeyModifiers"] = [this]() -> sol::table {
            SDL_Keymod mod = SDL_GetModState();
            sol::table result = lua.create_table();
            result["shift"] = (mod & SDL_KMOD_SHIFT) != 0;
            result["ctrl"] = (mod & SDL_KMOD_CTRL) != 0;
            result["alt"] = (mod & SDL_KMOD_ALT) != 0;
            result["gui"] = (mod & SDL_KMOD_GUI) != 0;
            return result;
        };

        // Drawing helper functions
        lua["drawLine"] = [this](float x1, float y1, float x2, float y2, float r, float g, float b, float a) {
            if (renderer) {
                SDL_SetRenderDrawColor(renderer,
                    static_cast<Uint8>(r * 255),
                    static_cast<Uint8>(g * 255),
                    static_cast<Uint8>(b * 255),
                    static_cast<Uint8>(a * 255));
                SDL_RenderLine(renderer, x1, y1, x2, y2);
            }
        };

        lua["drawRectOutline"] = [this](float x, float y, float w, float h, float r, float g, float b, float a) {
            if (renderer) {
                SDL_SetRenderDrawColor(renderer,
                    static_cast<Uint8>(r * 255),
                    static_cast<Uint8>(g * 255),
                    static_cast<Uint8>(b * 255),
                    static_cast<Uint8>(a * 255));
                SDL_FRect rect = {x, y, w, h};
                SDL_RenderRect(renderer, &rect);
            }
        };

        // Text measurement helpers for cursor positioning
        lua["measureTextToOffset"] = [this](const std::string& text, int byteOffset) -> int {
            if (!currentFont) return 0;
            if (byteOffset <= 0) return 0;
            if (byteOffset >= static_cast<int>(text.length())) {
                int w = 0, h = 0;
                TTF_GetStringSize(currentFont, text.c_str(), text.length(), &w, &h);
                return w;
            }

            int w = 0, h = 0;
            TTF_GetStringSize(currentFont, text.c_str(), byteOffset, &w, &h);
            return w;
        };

        lua["getOffsetFromX"] = [this](const std::string& text, float targetX) -> int {
            if (!currentFont || text.empty()) return 0;
            if (targetX <= 0) return 0;

            // Binary search for the byte offset closest to targetX
            int low = 0;
            int high = static_cast<int>(text.length());

            while (low < high) {
                int mid = (low + high + 1) / 2;
                int w = 0, h = 0;
                TTF_GetStringSize(currentFont, text.c_str(), mid, &w, &h);

                if (w <= targetX) {
                    low = mid;
                } else {
                    high = mid - 1;
                }
            }

            // Check if we should snap to the next character
            if (low < static_cast<int>(text.length())) {
                int wLow = 0, wNext = 0, h = 0;
                TTF_GetStringSize(currentFont, text.c_str(), low, &wLow, &h);
                TTF_GetStringSize(currentFont, text.c_str(), low + 1, &wNext, &h);
                float midPoint = (wLow + wNext) / 2.0f;
                if (targetX > midPoint) {
                    return low + 1;
                }
            }

            return low;
        };
    }

    bool loadScript(const std::string& scriptPath) {
        try {
            lua.script_file(scriptPath);
            std::cout << "Loaded script: " << scriptPath << std::endl;
            return true;
        } catch (const sol::error& e) {
            std::cerr << "Lua script error: " << e.what() << std::endl;
            return false;
        }
    }

    void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                windowWidth = event.window.data1;
                windowHeight = event.window.data2;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                // Call Lua onKeyDown if it exists
                sol::optional<sol::function> onKeyDown = lua["onKeyDown"];
                if (onKeyDown) {
                    try {
                        (*onKeyDown)(SDL_GetKeyName(event.key.key));
                    } catch (const sol::error& e) {
                        std::cerr << "Lua onKeyDown error: " << e.what() << std::endl;
                    }
                }
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                // Call Lua onMouseDown if it exists
                sol::optional<sol::function> onMouseDown = lua["onMouseDown"];
                if (onMouseDown) {
                    try {
                        (*onMouseDown)(event.button.x, event.button.y, event.button.button);
                    } catch (const sol::error& e) {
                        std::cerr << "Lua onMouseDown error: " << e.what() << std::endl;
                    }
                }
            } else if (event.type == SDL_EVENT_KEY_UP) {
                // Call Lua onKeyUp if it exists
                sol::optional<sol::function> onKeyUp = lua["onKeyUp"];
                if (onKeyUp) {
                    try {
                        (*onKeyUp)(SDL_GetKeyName(event.key.key));
                    } catch (const sol::error& e) {
                        std::cerr << "Lua onKeyUp error: " << e.what() << std::endl;
                    }
                }
            } else if (event.type == SDL_EVENT_TEXT_INPUT) {
                // Call Lua onTextInput if it exists
                sol::optional<sol::function> onTextInput = lua["onTextInput"];
                if (onTextInput) {
                    try {
                        (*onTextInput)(event.text.text);
                    } catch (const sol::error& e) {
                        std::cerr << "Lua onTextInput error: " << e.what() << std::endl;
                    }
                }
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                // Call Lua onMouseUp if it exists
                sol::optional<sol::function> onMouseUp = lua["onMouseUp"];
                if (onMouseUp) {
                    try {
                        (*onMouseUp)(event.button.x, event.button.y, event.button.button);
                    } catch (const sol::error& e) {
                        std::cerr << "Lua onMouseUp error: " << e.what() << std::endl;
                    }
                }
            } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                // Call Lua onMouseWheel if it exists
                sol::optional<sol::function> onMouseWheel = lua["onMouseWheel"];
                if (onMouseWheel) {
                    try {
                        float mouseX, mouseY;
                        SDL_GetMouseState(&mouseX, &mouseY);
                        (*onMouseWheel)(mouseX, mouseY, event.wheel.x, event.wheel.y);
                    } catch (const sol::error& e) {
                        std::cerr << "Lua onMouseWheel error: " << e.what() << std::endl;
                    }
                }
            } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                // Call Lua onMouseMove if it exists
                sol::optional<sol::function> onMouseMove = lua["onMouseMove"];
                if (onMouseMove) {
                    try {
                        (*onMouseMove)(event.motion.x, event.motion.y);
                    } catch (const sol::error& e) {
                        std::cerr << "Lua onMouseMove error: " << e.what() << std::endl;
                    }
                }
            } else if (event.type == SDL_EVENT_FINGER_DOWN) {
                // Call Lua onTouchDown if it exists
                sol::optional<sol::function> onTouchDown = lua["onTouchDown"];
                if (onTouchDown) {
                    try {
                        float x = event.tfinger.x * windowWidth;
                        float y = event.tfinger.y * windowHeight;
                        (*onTouchDown)(event.tfinger.fingerID, x, y, event.tfinger.pressure);
                    } catch (const sol::error& e) {
                        std::cerr << "Lua onTouchDown error: " << e.what() << std::endl;
                    }
                }
                // Also call onMouseDown for compatibility
                sol::optional<sol::function> onMouseDown = lua["onMouseDown"];
                if (onMouseDown) {
                    try {
                        float x = event.tfinger.x * windowWidth;
                        float y = event.tfinger.y * windowHeight;
                        (*onMouseDown)(x, y, 1); // Treat as left click
                    } catch (const sol::error& e) {
                        std::cerr << "Lua onMouseDown error: " << e.what() << std::endl;
                    }
                }
            } else if (event.type == SDL_EVENT_FINGER_UP) {
                // Call Lua onTouchUp if it exists
                sol::optional<sol::function> onTouchUp = lua["onTouchUp"];
                if (onTouchUp) {
                    try {
                        float x = event.tfinger.x * windowWidth;
                        float y = event.tfinger.y * windowHeight;
                        (*onTouchUp)(event.tfinger.fingerID, x, y);
                    } catch (const sol::error& e) {
                        std::cerr << "Lua onTouchUp error: " << e.what() << std::endl;
                    }
                }
            } else if (event.type == SDL_EVENT_FINGER_MOTION) {
                // Call Lua onTouchMove if it exists
                sol::optional<sol::function> onTouchMove = lua["onTouchMove"];
                if (onTouchMove) {
                    try {
                        float x = event.tfinger.x * windowWidth;
                        float y = event.tfinger.y * windowHeight;
                        float dx = event.tfinger.dx * windowWidth;
                        float dy = event.tfinger.dy * windowHeight;
                        (*onTouchMove)(event.tfinger.fingerID, x, y, dx, dy);
                    } catch (const sol::error& e) {
                        std::cerr << "Lua onTouchMove error: " << e.what() << std::endl;
                    }
                }
            }
        }
    }

    void update(float deltaTime) {
        // Call Lua update function if it exists
        sol::optional<sol::function> updateFunc = lua["update"];
        if (updateFunc) {
            try {
                (*updateFunc)(deltaTime);
            } catch (const sol::error& e) {
                std::cerr << "Lua update error: " << e.what() << std::endl;
            }
        }
    }

    void render() {
        SDL_SetRenderDrawColorFloat(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderClear(renderer);

        // Call Lua render function if it exists
        sol::optional<sol::function> renderFunc = lua["render"];
        if (renderFunc) {
            try {
                (*renderFunc)();
            } catch (const sol::error& e) {
                std::cerr << "Lua render error: " << e.what() << std::endl;
            }
        }

        SDL_RenderPresent(renderer);
    }

    void run() {
        Uint64 lastTime = SDL_GetTicks();

        while (running) {
            Uint64 currentTime = SDL_GetTicks();
            float deltaTime = (currentTime - lastTime) / 1000.0f;
            lastTime = currentTime;

            handleEvents();
            update(deltaTime);
            render();

            SDL_Delay(16); // ~60 FPS
        }
    }

    void cleanup() {
        // Cleanup fonts
        for (auto& [id, entry] : fonts) {
            for (auto& [size, font] : entry.sizeCache) {
                TTF_CloseFont(font);
            }
        }
        fonts.clear();
        currentFont = nullptr;
        currentFontId = 0;

        // Cleanup TTF
        if (textEngine) {
            TTF_DestroyRendererTextEngine(textEngine);
            textEngine = nullptr;
        }
        TTF_Quit();

        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        SDL_Quit();
    }
};

int main(int argc, char* argv[]) {
    Application app;

    if (!app.initialize()) {
        return 1;
    }

    // Load the Lua script (use command-line argument or default to main.lua)
    std::string scriptPath = (argc > 1) ? argv[1] : "scripts/main.lua";
    if (!app.loadScript(scriptPath)) {
        std::cerr << "Failed to load " << scriptPath << std::endl;
        return 1;
    }

    app.run();

    return 0;
}
