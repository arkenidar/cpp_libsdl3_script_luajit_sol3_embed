#include "EventHandler.hpp"
#include "../widgets/TextWidget.hpp"
#include <iostream>

EventHandler::EventHandler(sol::state& luaState,
                           std::map<int, std::shared_ptr<TextWidget>>& widgets,
                           SDL_Window* win,
                           bool& runningFlag,
                           int& winWidth,
                           int& winHeight)
    : lua(luaState)
    , textWidgets(widgets)
    , window(win)
    , running(runningFlag)
    , windowWidth(winWidth)
    , windowHeight(winHeight)
{
}

void EventHandler::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                handleQuit();
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                handleWindowResize(event);
                break;
            case SDL_EVENT_KEY_DOWN:
                handleKeyDown(event);
                break;
            case SDL_EVENT_KEY_UP:
                handleKeyUp(event);
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                handleMouseButtonDown(event);
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                handleMouseButtonUp(event);
                break;
            case SDL_EVENT_MOUSE_MOTION:
                handleMouseMotion(event);
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                handleMouseWheel(event);
                break;
            case SDL_EVENT_TEXT_INPUT:
                handleTextInput(event);
                break;
            case SDL_EVENT_FINGER_DOWN:
                handleFingerDown(event);
                break;
            case SDL_EVENT_FINGER_UP:
                handleFingerUp(event);
                break;
            case SDL_EVENT_FINGER_MOTION:
                handleFingerMotion(event);
                break;
        }
    }
}

void EventHandler::handleQuit() {
    running = false;
}

void EventHandler::handleWindowResize(const SDL_Event& event) {
    windowWidth = event.window.data1;
    windowHeight = event.window.data2;
}

void EventHandler::handleKeyDown(const SDL_Event& event) {
    // Route to widgets first
    std::string keyName = SDL_GetKeyName(event.key.key);
    SDL_Keymod mod = SDL_GetModState();
    bool shift = (mod & SDL_KMOD_SHIFT) != 0;
    bool ctrl = (mod & SDL_KMOD_CTRL) != 0;
    bool consumed = false;

    for (auto& [id, widget] : textWidgets) {
        if (widget->handleKeyDown(keyName, shift, ctrl)) {
            consumed = true;
            break;
        }
    }

    // Call Lua onKeyDown if not consumed by widget
    if (!consumed) {
        sol::optional<sol::function> onKeyDown = lua["onKeyDown"];
        if (onKeyDown) {
            try {
                (*onKeyDown)(keyName);
            } catch (const sol::error& e) {
                std::cerr << "Lua onKeyDown error: " << e.what() << std::endl;
            }
        }
    }
}

void EventHandler::handleKeyUp(const SDL_Event& event) {
    // Call Lua onKeyUp if it exists
    sol::optional<sol::function> onKeyUp = lua["onKeyUp"];
    if (onKeyUp) {
        try {
            (*onKeyUp)(SDL_GetKeyName(event.key.key));
        } catch (const sol::error& e) {
            std::cerr << "Lua onKeyUp error: " << e.what() << std::endl;
        }
    }
}

void EventHandler::handleMouseButtonDown(const SDL_Event& event) {
    // First, unfocus all widgets so only the clicked one will have focus
    for (auto& [id, widget] : textWidgets) {
        if (widget->hasFocus() && !widget->hitTest(event.button.x, event.button.y)) {
            widget->setFocus(false);
        }
    }

    // Now handle the click
    bool consumed = false;
    for (auto& [id, widget] : textWidgets) {
        if (widget->handleMouseDown(event.button.x, event.button.y, event.button.button)) {
            consumed = true;
            break;
        }
    }

    // Call Lua onMouseDown if not consumed
    if (!consumed) {
        sol::optional<sol::function> onMouseDown = lua["onMouseDown"];
        if (onMouseDown) {
            try {
                (*onMouseDown)(event.button.x, event.button.y, event.button.button);
            } catch (const sol::error& e) {
                std::cerr << "Lua onMouseDown error: " << e.what() << std::endl;
            }
        }
    }
}

void EventHandler::handleMouseButtonUp(const SDL_Event& event) {
    // Route to widgets first
    for (auto& [id, widget] : textWidgets) {
        widget->handleMouseUp(event.button.x, event.button.y, event.button.button);
    }

    // Always call Lua onMouseUp
    sol::optional<sol::function> onMouseUp = lua["onMouseUp"];
    if (onMouseUp) {
        try {
            (*onMouseUp)(event.button.x, event.button.y, event.button.button);
        } catch (const sol::error& e) {
            std::cerr << "Lua onMouseUp error: " << e.what() << std::endl;
        }
    }
}

void EventHandler::handleMouseMotion(const SDL_Event& event) {
    // Route to widgets first (for drag selection)
    for (auto& [id, widget] : textWidgets) {
        widget->handleMouseMove(event.motion.x, event.motion.y);
    }

    // Always call Lua onMouseMove
    sol::optional<sol::function> onMouseMove = lua["onMouseMove"];
    if (onMouseMove) {
        try {
            (*onMouseMove)(event.motion.x, event.motion.y);
        } catch (const sol::error& e) {
            std::cerr << "Lua onMouseMove error: " << e.what() << std::endl;
        }
    }
}

void EventHandler::handleMouseWheel(const SDL_Event& event) {
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
}

void EventHandler::handleTextInput(const SDL_Event& event) {
    // Route to widgets first
    bool consumed = false;
    for (auto& [id, widget] : textWidgets) {
        if (widget->handleTextInput(event.text.text)) {
            consumed = true;
            break;
        }
    }

    // Call Lua onTextInput if not consumed
    if (!consumed) {
        sol::optional<sol::function> onTextInput = lua["onTextInput"];
        if (onTextInput) {
            try {
                (*onTextInput)(event.text.text);
            } catch (const sol::error& e) {
                std::cerr << "Lua onTextInput error: " << e.what() << std::endl;
            }
        }
    }
}

void EventHandler::handleFingerDown(const SDL_Event& event) {
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
}

void EventHandler::handleFingerUp(const SDL_Event& event) {
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
}

void EventHandler::handleFingerMotion(const SDL_Event& event) {
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
