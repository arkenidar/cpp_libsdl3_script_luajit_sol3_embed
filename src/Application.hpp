#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <sol/sol.hpp>
#include <map>
#include <memory>
#include <string>

#include "widgets/TextWidget.hpp"
#include "graphics/FontManager.hpp"
#include "events/EventHandler.hpp"

// Forward declaration for friend class
class LuaBindings;

class Application {
private:
    // Grant LuaBindings access to private members
    friend class LuaBindings;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool running = true;
    sol::state lua;

    int windowWidth = 800;
    int windowHeight = 600;
    SDL_FColor bgColor = {0.1f, 0.1f, 0.15f, 1.0f};

    // TTF text rendering
    TTF_TextEngine* textEngine = nullptr;

    // Font management
    FontManager fontManager;

    // TextWidget management
    std::map<int, std::shared_ptr<TextWidget>> textWidgets;
    int nextWidgetId = 1;

    // Event handling
    std::unique_ptr<EventHandler> eventHandler;

public:
    Application();
    ~Application();

    bool initialize();
    bool loadScript(const std::string& scriptPath);
    void update(float deltaTime);
    void render();
    void run();
    void cleanup();
};

#endif // APPLICATION_HPP
