#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <sol/sol.hpp>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <cmath>
#include <memory>

// Forward declaration
class Application;

// TextWidget class for text input/display
class TextWidget {
public:
    // Configuration
    float x = 0, y = 0;
    float width = 200, height = 30;
    bool multiline = false;
    bool editable = true;

    // Colors (normalized 0-1)
    struct Colors {
        float bgR = 0.15f, bgG = 0.15f, bgB = 0.2f, bgA = 1.0f;
        float textR = 1.0f, textG = 1.0f, textB = 1.0f, textA = 1.0f;
        float cursorR = 1.0f, cursorG = 1.0f, cursorB = 1.0f, cursorA = 1.0f;
        float selectionR = 0.3f, selectionG = 0.5f, selectionB = 0.8f, selectionA = 0.5f;
        float borderR = 0.4f, borderG = 0.4f, borderB = 0.5f, borderA = 1.0f;
        float focusBorderR = 0.3f, focusBorderG = 0.6f, focusBorderB = 1.0f, focusBorderA = 1.0f;
    } colors;

    // Padding
    float paddingX = 8.0f;
    float paddingY = 6.0f;

private:
    // State
    std::string text;
    int cursorPos = 0;           // Byte offset in text
    int selectionStart = -1;     // -1 means no selection
    int selectionEnd = -1;
    bool focused = false;
    float cursorBlink = 0.0f;
    float scrollX = 0.0f;
    float scrollY = 0.0f;
    bool isDragging = false;

    // References (set by Application)
    SDL_Renderer* renderer = nullptr;
    TTF_TextEngine* textEngine = nullptr;
    TTF_Font* font = nullptr;
    int fontHeight = 16;
    SDL_Window* window = nullptr;

    // Helper: Get width of text substring
    int getTextWidth(const std::string& str, size_t len) {
        if (!font || len == 0) return 0;
        int w = 0, h = 0;
        TTF_GetStringSize(font, str.c_str(), len, &w, &h);
        return w;
    }

    // Helper: Get byte offset from X position
    int getOffsetFromX(const std::string& str, float targetX) {
        if (!font || str.empty()) return 0;
        if (targetX <= 0) return 0;

        int low = 0;
        int high = static_cast<int>(str.length());

        while (low < high) {
            int mid = (low + high + 1) / 2;
            int w = getTextWidth(str, mid);
            if (w <= targetX) {
                low = mid;
            } else {
                high = mid - 1;
            }
        }

        // Snap to closer character
        if (low < static_cast<int>(str.length())) {
            int wLow = getTextWidth(str, low);
            int wNext = getTextWidth(str, low + 1);
            float midPoint = (wLow + wNext) / 2.0f;
            if (targetX > midPoint) {
                return low + 1;
            }
        }
        return low;
    }

    // Helper: Get line info for multiline text
    struct LineInfo {
        int start;      // Byte offset of line start
        int length;     // Byte length of line (excluding newline)
    };

    std::vector<LineInfo> getLines() {
        std::vector<LineInfo> lines;
        int start = 0;
        for (size_t i = 0; i <= text.length(); i++) {
            if (i == text.length() || text[i] == '\n') {
                lines.push_back({start, static_cast<int>(i - start)});
                start = static_cast<int>(i + 1);
            }
        }
        return lines;
    }

    // Get current line index and position within line
    std::pair<int, int> getCursorLineInfo() {
        auto lines = getLines();
        int pos = 0;
        for (size_t i = 0; i < lines.size(); i++) {
            int lineEnd = pos + lines[i].length;
            if (cursorPos <= lineEnd || i == lines.size() - 1) {
                return {static_cast<int>(i), cursorPos - pos};
            }
            pos = lineEnd + 1; // +1 for newline
        }
        return {0, 0};
    }

    // Move cursor to specific line and column
    void moveCursorToLine(int lineIdx, int col) {
        auto lines = getLines();
        if (lineIdx < 0) lineIdx = 0;
        if (lineIdx >= static_cast<int>(lines.size())) lineIdx = static_cast<int>(lines.size()) - 1;

        int pos = 0;
        for (int i = 0; i < lineIdx; i++) {
            pos += lines[i].length + 1; // +1 for newline
        }
        col = std::max(0, std::min(col, lines[lineIdx].length));
        cursorPos = pos + col;
    }

    // Clear selection
    void clearSelection() {
        selectionStart = -1;
        selectionEnd = -1;
    }

    // Get ordered selection range
    std::pair<int, int> getSelectionRange() {
        if (selectionStart < 0) return {-1, -1};
        return {std::min(selectionStart, selectionEnd), std::max(selectionStart, selectionEnd)};
    }

    // Delete selected text
    void deleteSelection() {
        auto [start, end] = getSelectionRange();
        if (start >= 0) {
            text.erase(start, end - start);
            cursorPos = start;
            clearSelection();
        }
    }

    // Get selected text
    std::string getSelectedText() {
        auto [start, end] = getSelectionRange();
        if (start >= 0) {
            return text.substr(start, end - start);
        }
        return "";
    }

    // Ensure cursor is visible (auto-scroll)
    void ensureCursorVisible() {
        if (!font) return;

        float contentWidth = width - paddingX * 2;
        float contentHeight = height - paddingY * 2;

        // For multiline, we need cursor position within the current line
        int cursorXInLine = 0;
        int lineIdx = 0;

        if (multiline) {
            auto lines = getLines();
            int pos = 0;
            for (size_t i = 0; i < lines.size(); i++) {
                int lineEnd = pos + lines[i].length;
                if (cursorPos <= lineEnd || i == lines.size() - 1) {
                    lineIdx = static_cast<int>(i);
                    int colInLine = cursorPos - pos;
                    std::string lineText = text.substr(pos, lines[i].length);
                    cursorXInLine = getTextWidth(lineText, colInLine);
                    break;
                }
                pos = lineEnd + 1;
            }
        } else {
            cursorXInLine = getTextWidth(text, cursorPos);
        }

        // Horizontal scrolling
        if (cursorXInLine - scrollX < 0) {
            scrollX = static_cast<float>(cursorXInLine);
        } else if (cursorXInLine - scrollX > contentWidth) {
            scrollX = cursorXInLine - contentWidth;
        }

        // Vertical scrolling (multiline only)
        if (multiline) {
            float cursorY = lineIdx * fontHeight;

            if (cursorY - scrollY < 0) {
                scrollY = cursorY;
            } else if (cursorY + fontHeight - scrollY > contentHeight) {
                scrollY = cursorY + fontHeight - contentHeight;
            }
        }

        // Clamp scroll values to valid range
        if (scrollX < 0) scrollX = 0;
        if (scrollY < 0) scrollY = 0;
    }

public:
    TextWidget() = default;

    void init(SDL_Renderer* r, TTF_TextEngine* te, TTF_Font* f, SDL_Window* w) {
        renderer = r;
        textEngine = te;
        font = f;
        window = w;
        if (font) {
            fontHeight = TTF_GetFontHeight(font);
        }
    }

    void setFont(TTF_Font* f) {
        font = f;
        if (font) {
            fontHeight = TTF_GetFontHeight(font);
        }
    }

    void setText(const std::string& t) {
        text = t;
        cursorPos = std::min(cursorPos, static_cast<int>(text.length()));
        clearSelection();
        ensureCursorVisible();
    }

    std::string getText() const { return text; }

    void setPosition(float newX, float newY) {
        x = newX;
        y = newY;
    }

    void setSize(float w, float h) {
        width = w;
        height = h;
    }

    void setMultiline(bool m) { multiline = m; }
    void setEditable(bool e) { editable = e; }

    void setFocus(bool f) {
        if (f != focused) {
            focused = f;
            cursorBlink = 0.0f;
            if (focused && window) {
                SDL_StartTextInput(window);
            } else if (!focused && window) {
                SDL_StopTextInput(window);
            }
        }
    }

    bool hasFocus() const { return focused; }

    bool hitTest(float px, float py) {
        return px >= x && px < x + width && py >= y && py < y + height;
    }

    void update(float dt) {
        if (focused) {
            cursorBlink += dt;
            if (cursorBlink > 1.0f) cursorBlink -= 1.0f;
        }
    }

    // Event handlers - return true if event was consumed
    bool handleMouseDown(float mx, float my, int button) {
        if (!hitTest(mx, my)) {
            if (focused) setFocus(false);
            return false;
        }

        setFocus(true);
        cursorBlink = 0.0f;

        // Calculate click position in text
        float localX = mx - x - paddingX + scrollX;
        float localY = my - y - paddingY + scrollY;

        if (multiline) {
            auto lines = getLines();
            int lineIdx = std::max(0, std::min(static_cast<int>(localY / fontHeight), static_cast<int>(lines.size()) - 1));
            int lineStart = 0;
            for (int i = 0; i < lineIdx; i++) {
                lineStart += lines[i].length + 1;
            }
            std::string lineText = text.substr(lineStart, lines[lineIdx].length);
            int col = getOffsetFromX(lineText, localX);
            cursorPos = lineStart + col;
        } else {
            cursorPos = getOffsetFromX(text, localX);
        }

        // Start selection on shift+click, otherwise clear
        SDL_Keymod mod = SDL_GetModState();
        if (mod & SDL_KMOD_SHIFT) {
            if (selectionStart < 0) selectionStart = cursorPos;
            selectionEnd = cursorPos;
        } else {
            clearSelection();
            selectionStart = cursorPos;
            selectionEnd = cursorPos;
        }

        isDragging = true;
        return true;
    }

    bool handleMouseUp(float mx, float my, int button) {
        isDragging = false;
        // If selection is empty (start == end), clear it
        if (selectionStart == selectionEnd) {
            clearSelection();
        }
        return focused;
    }

    bool handleMouseMove(float mx, float my) {
        if (!isDragging || !focused) return false;

        float localX = mx - x - paddingX + scrollX;
        float localY = my - y - paddingY + scrollY;

        if (multiline) {
            auto lines = getLines();
            int lineIdx = std::max(0, std::min(static_cast<int>(localY / fontHeight), static_cast<int>(lines.size()) - 1));
            int lineStart = 0;
            for (int i = 0; i < lineIdx; i++) {
                lineStart += lines[i].length + 1;
            }
            std::string lineText = text.substr(lineStart, lines[lineIdx].length);
            int col = getOffsetFromX(lineText, localX);
            cursorPos = lineStart + col;
        } else {
            cursorPos = getOffsetFromX(text, localX);
        }

        selectionEnd = cursorPos;
        ensureCursorVisible();
        return true;
    }

    bool handleKeyDown(const std::string& key, bool shift, bool ctrl) {
        if (!focused) return false;

        cursorBlink = 0.0f;

        // Navigation
        if (key == "Left") {
            if (shift) {
                if (selectionStart < 0) selectionStart = cursorPos;
            } else {
                if (selectionStart >= 0) {
                    cursorPos = getSelectionRange().first;
                    clearSelection();
                    ensureCursorVisible();
                    return true;
                }
            }
            if (cursorPos > 0) cursorPos--;
            if (shift) selectionEnd = cursorPos;
            else clearSelection();
            ensureCursorVisible();
            return true;
        }

        if (key == "Right") {
            if (shift) {
                if (selectionStart < 0) selectionStart = cursorPos;
            } else {
                if (selectionStart >= 0) {
                    cursorPos = getSelectionRange().second;
                    clearSelection();
                    ensureCursorVisible();
                    return true;
                }
            }
            if (cursorPos < static_cast<int>(text.length())) cursorPos++;
            if (shift) selectionEnd = cursorPos;
            else clearSelection();
            ensureCursorVisible();
            return true;
        }

        if (key == "Up" && multiline) {
            auto [lineIdx, col] = getCursorLineInfo();
            if (shift && selectionStart < 0) selectionStart = cursorPos;
            if (lineIdx > 0) {
                moveCursorToLine(lineIdx - 1, col);
            }
            if (shift) selectionEnd = cursorPos;
            else clearSelection();
            ensureCursorVisible();
            return true;
        }

        if (key == "Down" && multiline) {
            auto [lineIdx, col] = getCursorLineInfo();
            if (shift && selectionStart < 0) selectionStart = cursorPos;
            auto lines = getLines();
            if (lineIdx < static_cast<int>(lines.size()) - 1) {
                moveCursorToLine(lineIdx + 1, col);
            }
            if (shift) selectionEnd = cursorPos;
            else clearSelection();
            ensureCursorVisible();
            return true;
        }

        if (key == "Home") {
            if (shift && selectionStart < 0) selectionStart = cursorPos;
            if (multiline) {
                auto [lineIdx, _] = getCursorLineInfo();
                moveCursorToLine(lineIdx, 0);
            } else {
                cursorPos = 0;
            }
            if (shift) selectionEnd = cursorPos;
            else clearSelection();
            ensureCursorVisible();
            return true;
        }

        if (key == "End") {
            if (shift && selectionStart < 0) selectionStart = cursorPos;
            if (multiline) {
                auto [lineIdx, _] = getCursorLineInfo();
                auto lines = getLines();
                moveCursorToLine(lineIdx, lines[lineIdx].length);
            } else {
                cursorPos = static_cast<int>(text.length());
            }
            if (shift) selectionEnd = cursorPos;
            else clearSelection();
            ensureCursorVisible();
            return true;
        }

        // Ctrl+A - Select all
        if (ctrl && (key == "A" || key == "a")) {
            selectionStart = 0;
            selectionEnd = static_cast<int>(text.length());
            cursorPos = selectionEnd;
            return true;
        }

        // Clipboard operations
        if (ctrl && (key == "C" || key == "c")) {
            std::string selected = getSelectedText();
            if (!selected.empty()) {
                SDL_SetClipboardText(selected.c_str());
            }
            return true;
        }

        if (ctrl && (key == "X" || key == "x") && editable) {
            std::string selected = getSelectedText();
            if (!selected.empty()) {
                SDL_SetClipboardText(selected.c_str());
                deleteSelection();
            }
            return true;
        }

        if (ctrl && (key == "V" || key == "v") && editable) {
            char* clip = SDL_GetClipboardText();
            if (clip) {
                deleteSelection();
                std::string clipStr(clip);
                // Remove newlines if single-line
                if (!multiline) {
                    clipStr.erase(std::remove(clipStr.begin(), clipStr.end(), '\n'), clipStr.end());
                    clipStr.erase(std::remove(clipStr.begin(), clipStr.end(), '\r'), clipStr.end());
                }
                text.insert(cursorPos, clipStr);
                cursorPos += static_cast<int>(clipStr.length());
                SDL_free(clip);
                ensureCursorVisible();
            }
            return true;
        }

        // Editing
        if (key == "Backspace" && editable) {
            if (selectionStart >= 0) {
                deleteSelection();
            } else if (cursorPos > 0) {
                text.erase(cursorPos - 1, 1);
                cursorPos--;
            }
            ensureCursorVisible();
            return true;
        }

        if (key == "Delete" && editable) {
            if (selectionStart >= 0) {
                deleteSelection();
            } else if (cursorPos < static_cast<int>(text.length())) {
                text.erase(cursorPos, 1);
            }
            ensureCursorVisible();
            return true;
        }

        if (key == "Return" && editable && multiline) {
            deleteSelection();
            text.insert(cursorPos, 1, '\n');
            cursorPos++;
            ensureCursorVisible();
            return true;
        }

        return false;
    }

    bool handleTextInput(const std::string& inputText) {
        if (!focused || !editable) return false;

        deleteSelection();

        std::string toInsert = inputText;
        if (!multiline) {
            // Remove newlines for single-line
            toInsert.erase(std::remove(toInsert.begin(), toInsert.end(), '\n'), toInsert.end());
            toInsert.erase(std::remove(toInsert.begin(), toInsert.end(), '\r'), toInsert.end());
        }

        text.insert(cursorPos, toInsert);
        cursorPos += static_cast<int>(toInsert.length());
        cursorBlink = 0.0f;
        ensureCursorVisible();
        return true;
    }

    void render() {
        if (!renderer || !font || !textEngine) return;

        // Background
        SDL_SetRenderDrawColor(renderer,
            static_cast<Uint8>(colors.bgR * 255),
            static_cast<Uint8>(colors.bgG * 255),
            static_cast<Uint8>(colors.bgB * 255),
            static_cast<Uint8>(colors.bgA * 255));
        SDL_FRect bgRect = {x, y, width, height};
        SDL_RenderFillRect(renderer, &bgRect);

        // Border
        if (focused) {
            SDL_SetRenderDrawColor(renderer,
                static_cast<Uint8>(colors.focusBorderR * 255),
                static_cast<Uint8>(colors.focusBorderG * 255),
                static_cast<Uint8>(colors.focusBorderB * 255),
                static_cast<Uint8>(colors.focusBorderA * 255));
        } else {
            SDL_SetRenderDrawColor(renderer,
                static_cast<Uint8>(colors.borderR * 255),
                static_cast<Uint8>(colors.borderG * 255),
                static_cast<Uint8>(colors.borderB * 255),
                static_cast<Uint8>(colors.borderA * 255));
        }
        SDL_RenderRect(renderer, &bgRect);

        // Set clip rect for text area
        SDL_Rect clipRect = {
            static_cast<int>(x + 1),
            static_cast<int>(y + 1),
            static_cast<int>(width - 2),
            static_cast<int>(height - 2)
        };
        SDL_SetRenderClipRect(renderer, &clipRect);

        float textX = x + paddingX - scrollX;
        float textY = y + paddingY - scrollY;

        // Draw selection highlight
        auto [selStart, selEnd] = getSelectionRange();
        if (selStart >= 0 && selStart != selEnd) {
            SDL_SetRenderDrawColor(renderer,
                static_cast<Uint8>(colors.selectionR * 255),
                static_cast<Uint8>(colors.selectionG * 255),
                static_cast<Uint8>(colors.selectionB * 255),
                static_cast<Uint8>(colors.selectionA * 255));

            if (multiline) {
                auto lines = getLines();
                int pos = 0;
                for (size_t i = 0; i < lines.size(); i++) {
                    int lineEnd = pos + lines[i].length;
                    if (selEnd > pos && selStart < lineEnd + 1) {
                        int lineSelStart = std::max(selStart - pos, 0);
                        int lineSelEnd = std::min(selEnd - pos, lines[i].length);
                        std::string lineText = text.substr(pos, lines[i].length);
                        float selX1 = textX + getTextWidth(lineText, lineSelStart);
                        float selX2 = textX + getTextWidth(lineText, lineSelEnd);
                        SDL_FRect selRect = {selX1, textY + i * fontHeight, selX2 - selX1, static_cast<float>(fontHeight)};
                        SDL_RenderFillRect(renderer, &selRect);
                    }
                    pos = lineEnd + 1;
                }
            } else {
                float selX1 = textX + getTextWidth(text, selStart);
                float selX2 = textX + getTextWidth(text, selEnd);
                SDL_FRect selRect = {selX1, textY, selX2 - selX1, static_cast<float>(fontHeight)};
                SDL_RenderFillRect(renderer, &selRect);
            }
        }

        // Draw text
        if (!text.empty()) {
            if (multiline) {
                auto lines = getLines();
                int pos = 0;
                for (size_t i = 0; i < lines.size(); i++) {
                    if (lines[i].length > 0) {
                        std::string lineText = text.substr(pos, lines[i].length);
                        TTF_Text* ttfText = TTF_CreateText(textEngine, font, lineText.c_str(), lineText.length());
                        if (ttfText) {
                            SDL_Color color = {
                                static_cast<Uint8>(colors.textR * 255),
                                static_cast<Uint8>(colors.textG * 255),
                                static_cast<Uint8>(colors.textB * 255),
                                static_cast<Uint8>(colors.textA * 255)
                            };
                            TTF_SetTextColor(ttfText, color.r, color.g, color.b, color.a);
                            TTF_DrawRendererText(ttfText, textX, textY + i * fontHeight);
                            TTF_DestroyText(ttfText);
                        }
                    }
                    pos += lines[i].length + 1;
                }
            } else {
                TTF_Text* ttfText = TTF_CreateText(textEngine, font, text.c_str(), text.length());
                if (ttfText) {
                    SDL_Color color = {
                        static_cast<Uint8>(colors.textR * 255),
                        static_cast<Uint8>(colors.textG * 255),
                        static_cast<Uint8>(colors.textB * 255),
                        static_cast<Uint8>(colors.textA * 255)
                    };
                    TTF_SetTextColor(ttfText, color.r, color.g, color.b, color.a);
                    TTF_DrawRendererText(ttfText, textX, textY);
                    TTF_DestroyText(ttfText);
                }
            }
        }

        // Draw cursor
        if (focused && cursorBlink < 0.5f) {
            float cursorX, cursorY;
            if (multiline) {
                auto [lineIdx, col] = getCursorLineInfo();
                auto lines = getLines();
                int lineStart = 0;
                for (int i = 0; i < lineIdx; i++) {
                    lineStart += lines[i].length + 1;
                }
                std::string lineText = text.substr(lineStart, col);
                cursorX = textX + getTextWidth(lineText, col);
                cursorY = textY + lineIdx * fontHeight;
            } else {
                cursorX = textX + getTextWidth(text, cursorPos);
                cursorY = textY;
            }

            SDL_SetRenderDrawColor(renderer,
                static_cast<Uint8>(colors.cursorR * 255),
                static_cast<Uint8>(colors.cursorG * 255),
                static_cast<Uint8>(colors.cursorB * 255),
                static_cast<Uint8>(colors.cursorA * 255));
            SDL_RenderLine(renderer, cursorX, cursorY + 2, cursorX, cursorY + fontHeight - 2);
        }

        // Reset clip rect
        SDL_SetRenderClipRect(renderer, nullptr);
    }
};

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

    // TextWidget management
    std::map<int, std::shared_ptr<TextWidget>> textWidgets;
    int nextWidgetId = 1;

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

        // TextWidget API
        lua["createTextWidget"] = [this](sol::table config) -> sol::object {
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
            widget->init(renderer, textEngine, currentFont, window);

            // Store widget
            int widgetId = nextWidgetId++;
            textWidgets[widgetId] = widget;

            // Create Lua userdata table with methods
            sol::table widgetTable = lua.create_table();
            widgetTable["_id"] = widgetId;

            widgetTable["setText"] = [this](sol::table self, const std::string& text) {
                int id = self["_id"];
                auto it = textWidgets.find(id);
                if (it != textWidgets.end()) {
                    it->second->setText(text);
                }
            };

            widgetTable["getText"] = [this](sol::table self) -> std::string {
                int id = self["_id"];
                auto it = textWidgets.find(id);
                if (it != textWidgets.end()) {
                    return it->second->getText();
                }
                return "";
            };

            widgetTable["setPosition"] = [this](sol::table self, float x, float y) {
                int id = self["_id"];
                auto it = textWidgets.find(id);
                if (it != textWidgets.end()) {
                    it->second->setPosition(x, y);
                }
            };

            widgetTable["setSize"] = [this](sol::table self, float w, float h) {
                int id = self["_id"];
                auto it = textWidgets.find(id);
                if (it != textWidgets.end()) {
                    it->second->setSize(w, h);
                }
            };

            widgetTable["setMultiline"] = [this](sol::table self, bool multiline) {
                int id = self["_id"];
                auto it = textWidgets.find(id);
                if (it != textWidgets.end()) {
                    it->second->setMultiline(multiline);
                }
            };

            widgetTable["setEditable"] = [this](sol::table self, bool editable) {
                int id = self["_id"];
                auto it = textWidgets.find(id);
                if (it != textWidgets.end()) {
                    it->second->setEditable(editable);
                }
            };

            widgetTable["setFocus"] = [this](sol::table self, bool focus) {
                int id = self["_id"];
                auto it = textWidgets.find(id);
                if (it != textWidgets.end()) {
                    it->second->setFocus(focus);
                }
            };

            widgetTable["hasFocus"] = [this](sol::table self) -> bool {
                int id = self["_id"];
                auto it = textWidgets.find(id);
                if (it != textWidgets.end()) {
                    return it->second->hasFocus();
                }
                return false;
            };

            widgetTable["update"] = [this](sol::table self, float dt) {
                int id = self["_id"];
                auto it = textWidgets.find(id);
                if (it != textWidgets.end()) {
                    it->second->update(dt);
                }
            };

            widgetTable["render"] = [this](sol::table self) {
                int id = self["_id"];
                auto it = textWidgets.find(id);
                if (it != textWidgets.end()) {
                    it->second->render();
                }
            };

            widgetTable["destroy"] = [this](sol::table self) {
                int id = self["_id"];
                textWidgets.erase(id);
            };

            return sol::make_object(lua, widgetTable);
        };

        // Route events to widgets (called before Lua callbacks)
        lua["_routeWidgetMouseDown"] = [this](float x, float y, int button) -> bool {
            for (auto& [id, widget] : textWidgets) {
                if (widget->handleMouseDown(x, y, button)) {
                    return true;
                }
            }
            return false;
        };

        lua["_routeWidgetMouseUp"] = [this](float x, float y, int button) -> bool {
            for (auto& [id, widget] : textWidgets) {
                if (widget->handleMouseUp(x, y, button)) {
                    return true;
                }
            }
            return false;
        };

        lua["_routeWidgetMouseMove"] = [this](float x, float y) -> bool {
            for (auto& [id, widget] : textWidgets) {
                if (widget->handleMouseMove(x, y)) {
                    return true;
                }
            }
            return false;
        };

        lua["_routeWidgetKeyDown"] = [this](const std::string& key) -> bool {
            SDL_Keymod mod = SDL_GetModState();
            bool shift = (mod & SDL_KMOD_SHIFT) != 0;
            bool ctrl = (mod & SDL_KMOD_CTRL) != 0;
            for (auto& [id, widget] : textWidgets) {
                if (widget->handleKeyDown(key, shift, ctrl)) {
                    return true;
                }
            }
            return false;
        };

        lua["_routeWidgetTextInput"] = [this](const std::string& text) -> bool {
            for (auto& [id, widget] : textWidgets) {
                if (widget->handleTextInput(text)) {
                    return true;
                }
            }
            return false;
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
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                // Route to widgets first
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
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
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
