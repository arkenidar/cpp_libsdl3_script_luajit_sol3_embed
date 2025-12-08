#ifndef EVENTHANDLER_HPP
#define EVENTHANDLER_HPP

#include <SDL3/SDL.h>
#include <sol/sol.hpp>
#include <map>
#include <memory>

// Forward declaration
class TextWidget;

class EventHandler {
private:
    sol::state& lua;
    std::map<int, std::shared_ptr<TextWidget>>& textWidgets;
    SDL_Window* window;
    bool& running;
    int& windowWidth;
    int& windowHeight;

public:
    EventHandler(sol::state& luaState,
                 std::map<int, std::shared_ptr<TextWidget>>& widgets,
                 SDL_Window* win,
                 bool& runningFlag,
                 int& winWidth,
                 int& winHeight);

    // Process all SDL events
    void handleEvents();

private:
    // Helper methods for specific event types
    void handleQuit();
    void handleWindowResize(const SDL_Event& event);
    void handleKeyDown(const SDL_Event& event);
    void handleKeyUp(const SDL_Event& event);
    void handleMouseButtonDown(const SDL_Event& event);
    void handleMouseButtonUp(const SDL_Event& event);
    void handleMouseMotion(const SDL_Event& event);
    void handleMouseWheel(const SDL_Event& event);
    void handleTextInput(const SDL_Event& event);
    void handleFingerDown(const SDL_Event& event);
    void handleFingerUp(const SDL_Event& event);
    void handleFingerMotion(const SDL_Event& event);
};

#endif // EVENTHANDLER_HPP
