#include "Application.hpp"
#include "lua/LuaBindings.hpp"
#include <iostream>

Application::Application() {
    // Initialize Lua with standard libraries
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::string);

    // Expose SDL and application functions to Lua
    LuaBindings::setupBindings(this, lua);

    // Initialize event handler (after Lua and other members are ready)
    eventHandler = std::make_unique<EventHandler>(lua, textWidgets, window, running, windowWidth, windowHeight);
}

Application::~Application() {
    cleanup();
}

bool Application::initialize() {
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

bool Application::loadScript(const std::string& scriptPath) {
    try {
        lua.script_file(scriptPath);
        std::cout << "Loaded script: " << scriptPath << std::endl;
        return true;
    } catch (const sol::error& e) {
        std::cerr << "Lua script error: " << e.what() << std::endl;
        return false;
    }
}

void Application::update(float deltaTime) {
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

void Application::render() {
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

void Application::run() {
    Uint64 lastTime = SDL_GetTicks();

    while (running) {
        Uint64 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        eventHandler->handleEvents();
        update(deltaTime);
        render();

        SDL_Delay(16); // ~60 FPS
    }
}

void Application::cleanup() {
    // Cleanup fonts
    fontManager.cleanup();

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
