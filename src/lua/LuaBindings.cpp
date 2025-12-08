#include "LuaBindings.hpp"
#include "../Application.hpp"
#include <iostream>

void LuaBindings::setupBindings(Application* app, sol::state& lua) {
    // Expose quit function
    lua["quit"] = [app]() { app->running = false; };

    // Expose window functions
    lua["setWindowTitle"] = [app](const std::string& title) {
        if (app->window) {
            SDL_SetWindowTitle(app->window, title.c_str());
        }
    };

    lua["setBackgroundColor"] = sol::overload(
        [app](float r, float g, float b) {
            app->bgColor = {r, g, b, 1.0f};
        },
        [app](float r, float g, float b, float a) {
            app->bgColor = {r, g, b, a};
        }
    );

    lua["getWindowSize"] = [app, &lua]() -> sol::table {
        sol::table size = lua.create_table();
        size["width"] = app->windowWidth;
        size["height"] = app->windowHeight;
        return size;
    };

    // Expose drawing functions
    lua["drawRect"] = [app](float x, float y, float w, float h, float r, float g, float b, float a = 1.0f) {
        if (app->renderer) {
            SDL_SetRenderDrawColor(app->renderer,
                static_cast<Uint8>(r * 255),
                static_cast<Uint8>(g * 255),
                static_cast<Uint8>(b * 255),
                static_cast<Uint8>(a * 255));
            SDL_FRect rect = {x, y, w, h};
            SDL_RenderFillRect(app->renderer, &rect);
        }
    };

    // Expose print function
    lua["print"] = [](const std::string& msg) {
        std::cout << "[Lua] " << msg << std::endl;
    };

    // Font management functions
    lua["loadFont"] = [app, &lua](const std::string& path, float size) -> sol::object {
        int fontId = app->fontManager.loadFont(path, size);
        if (fontId < 0) {
            return sol::nil;
        }
        return sol::make_object(lua, fontId);
    };

    lua["setFont"] = [app](int fontId) -> bool {
        app->fontManager.setCurrentFont(fontId);
        return app->fontManager.getCurrentFontId() == fontId;
    };

    lua["setFontSize"] = [app](float size) -> bool {
        if (app->fontManager.getCurrentFontId() == 0) return false;
        app->fontManager.setCurrentFontSize(size);
        return app->fontManager.getCurrentFont(size) != nullptr;
    };

    lua["getFontSize"] = [app]() -> float {
        return app->fontManager.getCurrentFontSize();
    };

    lua["closeFont"] = [app](int fontId) {
        app->fontManager.closeFont(fontId);
    };

    // Text measurement functions
    lua["measureText"] = [app, &lua](const std::string& text) -> sol::table {
        sol::table result = lua.create_table();
        result["width"] = 0;
        result["height"] = 0;

        TTF_Font* font = app->fontManager.getCurrentFont(app->fontManager.getCurrentFontSize());
        if (!font) return result;

        int w = 0, h = 0;
        if (TTF_GetStringSize(font, text.c_str(), text.length(), &w, &h)) {
            result["width"] = w;
            result["height"] = h;
        }
        return result;
    };

    lua["getFontHeight"] = [app]() -> int {
        TTF_Font* font = app->fontManager.getCurrentFont(app->fontManager.getCurrentFontSize());
        if (!font) return 0;
        return TTF_GetFontHeight(font);
    };

    // Text rendering function
    lua["drawText"] = sol::overload(
        // drawText(text, x, y, r, g, b, a)
        [app](const std::string& text, float x, float y, float r, float g, float b, float a) {
            TTF_Font* font = app->fontManager.getCurrentFont(app->fontManager.getCurrentFontSize());
            if (!font || !app->textEngine || !app->renderer) return;

            TTF_Text* ttfText = TTF_CreateText(app->textEngine, font, text.c_str(), text.length());
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
        [app](const std::string& text, float x, float y, float r, float g, float b) {
            TTF_Font* font = app->fontManager.getCurrentFont(app->fontManager.getCurrentFontSize());
            if (!font || !app->textEngine || !app->renderer) return;

            TTF_Text* ttfText = TTF_CreateText(app->textEngine, font, text.c_str(), text.length());
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
        [app](const std::string& text, float x, float y, float size, float r, float g, float b, float a) {
            if (app->fontManager.getCurrentFontId() == 0 || !app->textEngine || !app->renderer) return;

            TTF_Font* font = app->fontManager.getFont(app->fontManager.getCurrentFontId(), size);
            if (!font) return;

            TTF_Text* ttfText = TTF_CreateText(app->textEngine, font, text.c_str(), text.length());
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
    lua["startTextInput"] = [app]() {
        if (app->window) {
            SDL_StartTextInput(app->window);
        }
    };

    lua["stopTextInput"] = [app]() {
        if (app->window) {
            SDL_StopTextInput(app->window);
        }
    };

    lua["isTextInputActive"] = [app]() -> bool {
        if (app->window) {
            return SDL_TextInputActive(app->window);
        }
        return false;
    };

    lua["setTextInputArea"] = [app](float x, float y, float w, float h, int cursorOffset) {
        if (app->window) {
            SDL_Rect rect = {
                static_cast<int>(x),
                static_cast<int>(y),
                static_cast<int>(w),
                static_cast<int>(h)
            };
            SDL_SetTextInputArea(app->window, &rect, cursorOffset);
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
    lua["getKeyModifiers"] = [&lua]() -> sol::table {
        SDL_Keymod mod = SDL_GetModState();
        sol::table result = lua.create_table();
        result["shift"] = (mod & SDL_KMOD_SHIFT) != 0;
        result["ctrl"] = (mod & SDL_KMOD_CTRL) != 0;
        result["alt"] = (mod & SDL_KMOD_ALT) != 0;
        result["gui"] = (mod & SDL_KMOD_GUI) != 0;
        return result;
    };

    // Drawing helper functions
    lua["drawLine"] = [app](float x1, float y1, float x2, float y2, float r, float g, float b, float a) {
        if (app->renderer) {
            SDL_SetRenderDrawColor(app->renderer,
                static_cast<Uint8>(r * 255),
                static_cast<Uint8>(g * 255),
                static_cast<Uint8>(b * 255),
                static_cast<Uint8>(a * 255));
            SDL_RenderLine(app->renderer, x1, y1, x2, y2);
        }
    };

    lua["drawRectOutline"] = [app](float x, float y, float w, float h, float r, float g, float b, float a) {
        if (app->renderer) {
            SDL_SetRenderDrawColor(app->renderer,
                static_cast<Uint8>(r * 255),
                static_cast<Uint8>(g * 255),
                static_cast<Uint8>(b * 255),
                static_cast<Uint8>(a * 255));
            SDL_FRect rect = {x, y, w, h};
            SDL_RenderRect(app->renderer, &rect);
        }
    };

    // Text measurement helpers for cursor positioning
    lua["measureTextToOffset"] = [app](const std::string& text, int byteOffset) -> int {
        TTF_Font* font = app->fontManager.getCurrentFont(app->fontManager.getCurrentFontSize());
        if (!font) return 0;
        if (byteOffset <= 0) return 0;
        if (byteOffset >= static_cast<int>(text.length())) {
            int w = 0, h = 0;
            TTF_GetStringSize(font, text.c_str(), text.length(), &w, &h);
            return w;
        }

        int w = 0, h = 0;
        TTF_GetStringSize(font, text.c_str(), byteOffset, &w, &h);
        return w;
    };

    lua["getOffsetFromX"] = [app](const std::string& text, float targetX) -> int {
        TTF_Font* font = app->fontManager.getCurrentFont(app->fontManager.getCurrentFontSize());
        if (!font || text.empty()) return 0;
        if (targetX <= 0) return 0;

        // Binary search for the byte offset closest to targetX
        int low = 0;
        int high = static_cast<int>(text.length());

        while (low < high) {
            int mid = (low + high + 1) / 2;
            int w = 0, h = 0;
            TTF_GetStringSize(font, text.c_str(), mid, &w, &h);

            if (w <= targetX) {
                low = mid;
            } else {
                high = mid - 1;
            }
        }

        // Check if we should snap to the next character
        if (low < static_cast<int>(text.length())) {
            int wLow = 0, wNext = 0, h = 0;
            TTF_GetStringSize(font, text.c_str(), low, &wLow, &h);
            TTF_GetStringSize(font, text.c_str(), low + 1, &wNext, &h);
            float midPoint = (wLow + wNext) / 2.0f;
            if (targetX > midPoint) {
                return low + 1;
            }
        }

        return low;
    };

    // TextWidget API
    lua["createTextWidget"] = [app, &lua](sol::table config) -> sol::object {
        auto widget = std::make_shared<TextWidget>();

        // Position and size
        widget->x = config.get_or("x", 0.0f);
        widget->y = config.get_or("y", 0.0f);
        widget->width = config.get_or("width", 200.0f);
        widget->height = config.get_or("height", 30.0f);

        // Options
        widget->multiline = config.get_or("multiline", false);
        widget->editable = config.get_or("editable", true);

        // Initialize with current renderer/font
        TTF_Font* font = app->fontManager.getCurrentFont(app->fontManager.getCurrentFontSize());
        widget->init(app->renderer, app->textEngine, font, app->window);

        // Store widget
        int widgetId = app->nextWidgetId++;
        app->textWidgets[widgetId] = widget;

        // Create Lua userdata table with methods
        sol::table widgetTable = lua.create_table();
        widgetTable["_id"] = widgetId;

        widgetTable["setText"] = [app](sol::table self, const std::string& text) {
            int id = self["_id"];
            auto it = app->textWidgets.find(id);
            if (it != app->textWidgets.end()) {
                it->second->setText(text);
            }
        };

        widgetTable["getText"] = [app](sol::table self) -> std::string {
            int id = self["_id"];
            auto it = app->textWidgets.find(id);
            if (it != app->textWidgets.end()) {
                return it->second->getText();
            }
            return "";
        };

        widgetTable["setPosition"] = [app](sol::table self, float x, float y) {
            int id = self["_id"];
            auto it = app->textWidgets.find(id);
            if (it != app->textWidgets.end()) {
                it->second->setPosition(x, y);
            }
        };

        widgetTable["setSize"] = [app](sol::table self, float w, float h) {
            int id = self["_id"];
            auto it = app->textWidgets.find(id);
            if (it != app->textWidgets.end()) {
                it->second->setSize(w, h);
            }
        };

        widgetTable["setMultiline"] = [app](sol::table self, bool multiline) {
            int id = self["_id"];
            auto it = app->textWidgets.find(id);
            if (it != app->textWidgets.end()) {
                it->second->setMultiline(multiline);
            }
        };

        widgetTable["setEditable"] = [app](sol::table self, bool editable) {
            int id = self["_id"];
            auto it = app->textWidgets.find(id);
            if (it != app->textWidgets.end()) {
                it->second->setEditable(editable);
            }
        };

        widgetTable["setFocus"] = [app](sol::table self, bool focus) {
            int id = self["_id"];
            auto it = app->textWidgets.find(id);
            if (it != app->textWidgets.end()) {
                it->second->setFocus(focus);
            }
        };

        widgetTable["hasFocus"] = [app](sol::table self) -> bool {
            int id = self["_id"];
            auto it = app->textWidgets.find(id);
            if (it != app->textWidgets.end()) {
                return it->second->hasFocus();
            }
            return false;
        };

        widgetTable["update"] = [app](sol::table self, float dt) {
            int id = self["_id"];
            auto it = app->textWidgets.find(id);
            if (it != app->textWidgets.end()) {
                it->second->update(dt);
            }
        };

        widgetTable["render"] = [app](sol::table self) {
            int id = self["_id"];
            auto it = app->textWidgets.find(id);
            if (it != app->textWidgets.end()) {
                it->second->render();
            }
        };

        widgetTable["destroy"] = [app](sol::table self) {
            int id = self["_id"];
            app->textWidgets.erase(id);
        };

        return sol::make_object(lua, widgetTable);
    };

    // Route events to widgets (called before Lua callbacks)
    lua["_routeWidgetMouseDown"] = [app](float x, float y, int button) -> bool {
        for (auto& [id, widget] : app->textWidgets) {
            if (widget->handleMouseDown(x, y, button)) {
                return true;
            }
        }
        return false;
    };

    lua["_routeWidgetMouseUp"] = [app](float x, float y, int button) -> bool {
        for (auto& [id, widget] : app->textWidgets) {
            if (widget->handleMouseUp(x, y, button)) {
                return true;
            }
        }
        return false;
    };

    lua["_routeWidgetMouseMove"] = [app](float x, float y) -> bool {
        for (auto& [id, widget] : app->textWidgets) {
            if (widget->handleMouseMove(x, y)) {
                return true;
            }
        }
        return false;
    };

    lua["_routeWidgetKeyDown"] = [app](const std::string& key) -> bool {
        SDL_Keymod mod = SDL_GetModState();
        bool shift = (mod & SDL_KMOD_SHIFT) != 0;
        bool ctrl = (mod & SDL_KMOD_CTRL) != 0;
        for (auto& [id, widget] : app->textWidgets) {
            if (widget->handleKeyDown(key, shift, ctrl)) {
                return true;
            }
        }
        return false;
    };

    lua["_routeWidgetTextInput"] = [app](const std::string& text) -> bool {
        for (auto& [id, widget] : app->textWidgets) {
            if (widget->handleTextInput(text)) {
                return true;
            }
        }
        return false;
    };
}
