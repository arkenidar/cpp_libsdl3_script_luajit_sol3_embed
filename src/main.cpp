#include <SDL3/SDL.h>
#include <sol/sol.hpp>
#include <iostream>
#include <string>

class Application {
private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool running = true;
    sol::state lua;

    int windowWidth = 800;
    int windowHeight = 600;
    SDL_FColor bgColor = {0.1f, 0.1f, 0.15f, 1.0f};

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
            } else if (event.type == SDL_EVENT_FINGER_DOWN) {
                // Call Lua onMouseDown for touch events (normalized to window coordinates)
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

    // Load the main Lua script
    if (!app.loadScript("scripts/main.lua")) {
        std::cerr << "Failed to load main.lua" << std::endl;
        return 1;
    }

    app.run();

    return 0;
}
