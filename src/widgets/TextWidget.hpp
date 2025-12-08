#ifndef TEXTWIDGET_HPP
#define TEXTWIDGET_HPP

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <vector>
#include <utility>

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

    // Undo/Redo stacks
    struct UndoState {
        std::string text;
        int cursorPos;
    };
    std::vector<UndoState> undoStack;
    std::vector<UndoState> redoStack;
    static const size_t MAX_UNDO_HISTORY = 100;

    // References (set by Application)
    SDL_Renderer* renderer = nullptr;
    TTF_TextEngine* textEngine = nullptr;
    TTF_Font* font = nullptr;
    int fontHeight = 16;
    SDL_Window* window = nullptr;

    // Helper: Get width of text substring
    int getTextWidth(const std::string& str, size_t len);

    // Helper: Get byte offset from X position
    int getOffsetFromX(const std::string& str, float targetX);

    // Helper: Get line info for multiline text
    struct LineInfo {
        int start;      // Byte offset of line start
        int length;     // Byte length of line (excluding newline)
    };

    std::vector<LineInfo> getLines();

    // Get current line index and position within line
    std::pair<int, int> getCursorLineInfo();

    // Move cursor to specific line and column
    void moveCursorToLine(int lineIdx, int col);

    // Clear selection
    void clearSelection();

    // Get ordered selection range
    std::pair<int, int> getSelectionRange();

    // Delete selected text
    void deleteSelection();

    // Get selected text
    std::string getSelectedText();

    // Save current state for undo
    void saveUndoState();

    // Undo last action
    void undo();

    // Redo last undone action
    void redo();

    // Ensure cursor is visible (auto-scroll)
    void ensureCursorVisible();

public:
    TextWidget() = default;

    void init(SDL_Renderer* r, TTF_TextEngine* te, TTF_Font* f, SDL_Window* w);

    void setFont(TTF_Font* f);

    void setText(const std::string& t);

    std::string getText() const;

    void setPosition(float newX, float newY);

    void setSize(float w, float h);

    void setMultiline(bool m);
    void setEditable(bool e);

    void setFocus(bool f);

    bool hasFocus() const;

    bool hitTest(float px, float py);

    void update(float dt);

    // Event handlers - return true if event was consumed
    bool handleMouseDown(float mx, float my, int button);

    bool handleMouseUp(float mx, float my, int button);

    bool handleMouseMove(float mx, float my);

    bool handleKeyDown(const std::string& key, bool shift, bool ctrl);

    bool handleTextInput(const std::string& inputText);

    void render();
};

#endif // TEXTWIDGET_HPP
