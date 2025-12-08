#ifndef FONTMANAGER_HPP
#define FONTMANAGER_HPP

#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <map>

class FontManager {
private:
    struct FontEntry {
        std::string path;
        std::map<float, TTF_Font*> sizeCache;  // size -> font instance
    };

    std::map<int, FontEntry> fonts;  // fontId -> FontEntry
    int nextFontId = 1;
    int currentFontId = 0;
    float currentFontSize = 16.0f;
    TTF_Font* currentFont = nullptr;

public:
    FontManager() = default;
    ~FontManager();

    // Load a font and return its ID
    int loadFont(const std::string& path, float size);

    // Get a font at a specific size
    TTF_Font* getFont(int fontId, float size);

    // Set the current font
    void setCurrentFont(int fontId);

    // Get the current font at a specific size
    TTF_Font* getCurrentFont(float size);

    // Get current font size
    float getCurrentFontSize() const { return currentFontSize; }

    // Set current font size
    void setCurrentFontSize(float size) { currentFontSize = size; }

    // Get current font ID
    int getCurrentFontId() const { return currentFontId; }

    // Close a specific font
    void closeFont(int fontId);

    // Close all fonts
    void cleanup();

private:
    // Helper to get or create a font at a specific size
    TTF_Font* getOrCreateFontAtSize(int fontId, float size);
};

#endif // FONTMANAGER_HPP
