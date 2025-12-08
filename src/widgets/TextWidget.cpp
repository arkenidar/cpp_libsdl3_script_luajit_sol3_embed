#include "TextWidget.hpp"
#include <algorithm>
#include <cmath>

// Helper: Get width of text substring
int TextWidget::getTextWidth(const std::string& str, size_t len) {
    if (!font || len == 0) return 0;
    int w = 0, h = 0;
    TTF_GetStringSize(font, str.c_str(), len, &w, &h);
    return w;
}

// Helper: Get byte offset from X position
int TextWidget::getOffsetFromX(const std::string& str, float targetX) {
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

std::vector<TextWidget::LineInfo> TextWidget::getLines() {
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
std::pair<int, int> TextWidget::getCursorLineInfo() {
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
void TextWidget::moveCursorToLine(int lineIdx, int col) {
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
void TextWidget::clearSelection() {
    selectionStart = -1;
    selectionEnd = -1;
}

// Get ordered selection range
std::pair<int, int> TextWidget::getSelectionRange() {
    if (selectionStart < 0) return {-1, -1};
    return {std::min(selectionStart, selectionEnd), std::max(selectionStart, selectionEnd)};
}

// Delete selected text
void TextWidget::deleteSelection() {
    auto [start, end] = getSelectionRange();
    if (start >= 0) {
        text.erase(start, end - start);
        cursorPos = start;
        clearSelection();
    }
}

// Get selected text
std::string TextWidget::getSelectedText() {
    auto [start, end] = getSelectionRange();
    if (start >= 0) {
        return text.substr(start, end - start);
    }
    return "";
}

// Save current state for undo
void TextWidget::saveUndoState() {
    // Don't save if text hasn't changed from last undo state
    if (!undoStack.empty() && undoStack.back().text == text) {
        return;
    }
    undoStack.push_back({text, cursorPos});
    if (undoStack.size() > MAX_UNDO_HISTORY) {
        undoStack.erase(undoStack.begin());
    }
    // Clear redo stack when new action is performed
    redoStack.clear();
}

// Undo last action
void TextWidget::undo() {
    if (undoStack.empty()) return;

    // Save current state to redo stack
    redoStack.push_back({text, cursorPos});

    // Restore previous state
    UndoState state = undoStack.back();
    undoStack.pop_back();
    text = state.text;
    cursorPos = std::min(state.cursorPos, static_cast<int>(text.length()));
    clearSelection();
    ensureCursorVisible();
}

// Redo last undone action
void TextWidget::redo() {
    if (redoStack.empty()) return;

    // Save current state to undo stack
    undoStack.push_back({text, cursorPos});

    // Restore redo state
    UndoState state = redoStack.back();
    redoStack.pop_back();
    text = state.text;
    cursorPos = std::min(state.cursorPos, static_cast<int>(text.length()));
    clearSelection();
    ensureCursorVisible();
}

// Ensure cursor is visible (auto-scroll)
void TextWidget::ensureCursorVisible() {
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

void TextWidget::init(SDL_Renderer* r, TTF_TextEngine* te, TTF_Font* f, SDL_Window* w) {
    renderer = r;
    textEngine = te;
    font = f;
    window = w;
    if (font) {
        fontHeight = TTF_GetFontHeight(font);
    }
}

void TextWidget::setFont(TTF_Font* f) {
    font = f;
    if (font) {
        fontHeight = TTF_GetFontHeight(font);
    }
}

void TextWidget::setText(const std::string& t) {
    text = t;
    cursorPos = std::min(cursorPos, static_cast<int>(text.length()));
    clearSelection();
    ensureCursorVisible();
}

std::string TextWidget::getText() const { return text; }

void TextWidget::setPosition(float newX, float newY) {
    x = newX;
    y = newY;
}

void TextWidget::setSize(float w, float h) {
    width = w;
    height = h;
}

void TextWidget::setMultiline(bool m) { multiline = m; }
void TextWidget::setEditable(bool e) { editable = e; }

void TextWidget::setFocus(bool f) {
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

bool TextWidget::hasFocus() const { return focused; }

bool TextWidget::hitTest(float px, float py) {
    return px >= x && px < x + width && py >= y && py < y + height;
}

void TextWidget::update(float dt) {
    if (focused) {
        cursorBlink += dt;
        if (cursorBlink > 1.0f) cursorBlink -= 1.0f;
    }
}

// Event handlers - return true if event was consumed
bool TextWidget::handleMouseDown(float mx, float my, int button) {
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

bool TextWidget::handleMouseUp(float mx, float my, int button) {
    isDragging = false;
    // If selection is empty (start == end), clear it
    if (selectionStart == selectionEnd) {
        clearSelection();
    }
    return focused;
}

bool TextWidget::handleMouseMove(float mx, float my) {
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

bool TextWidget::handleKeyDown(const std::string& key, bool shift, bool ctrl) {
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

    // Undo/Redo
    if (ctrl && !shift && (key == "Z" || key == "z") && editable) {
        undo();
        return true;
    }

    if (ctrl && (key == "Y" || key == "y") && editable) {
        redo();
        return true;
    }

    if (ctrl && shift && (key == "Z" || key == "z") && editable) {
        redo();
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
            saveUndoState();
            SDL_SetClipboardText(selected.c_str());
            deleteSelection();
        }
        return true;
    }

    if (ctrl && (key == "V" || key == "v") && editable) {
        char* clip = SDL_GetClipboardText();
        if (clip) {
            saveUndoState();
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
        saveUndoState();
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
        saveUndoState();
        if (selectionStart >= 0) {
            deleteSelection();
        } else if (cursorPos < static_cast<int>(text.length())) {
            text.erase(cursorPos, 1);
        }
        ensureCursorVisible();
        return true;
    }

    if (key == "Return" && editable && multiline) {
        saveUndoState();
        deleteSelection();
        text.insert(cursorPos, 1, '\n');
        cursorPos++;
        ensureCursorVisible();
        return true;
    }

    return false;
}

bool TextWidget::handleTextInput(const std::string& inputText) {
    if (!focused || !editable) return false;

    saveUndoState();
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

void TextWidget::render() {
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
