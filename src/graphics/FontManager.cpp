#include "FontManager.hpp"
#include <iostream>
#include <SDL3/SDL.h>

FontManager::~FontManager() {
    cleanup();
}

int FontManager::loadFont(const std::string& path, float size) {
    TTF_Font* font = TTF_OpenFont(path.c_str(), size);
    if (!font) {
        std::cerr << "Failed to load font: " << path << " - " << SDL_GetError() << std::endl;
        return -1;
    }

    int fontId = nextFontId++;
    fonts[fontId] = FontEntry{path, {{size, font}}};

    // If no current font, set this as current
    if (currentFontId == 0) {
        currentFontId = fontId;
        currentFontSize = size;
        currentFont = font;
    }

    return fontId;
}

TTF_Font* FontManager::getFont(int fontId, float size) {
    return getOrCreateFontAtSize(fontId, size);
}

void FontManager::setCurrentFont(int fontId) {
    TTF_Font* font = getOrCreateFontAtSize(fontId, currentFontSize);
    if (font) {
        currentFontId = fontId;
        currentFont = font;
    }
}

TTF_Font* FontManager::getCurrentFont(float size) {
    if (currentFontId == 0) return nullptr;

    // If size is different from current, update current font size
    if (size != currentFontSize) {
        TTF_Font* font = getOrCreateFontAtSize(currentFontId, size);
        if (font) {
            currentFontSize = size;
            currentFont = font;
        }
    }

    return currentFont;
}

void FontManager::closeFont(int fontId) {
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
}

void FontManager::cleanup() {
    for (auto& [fontId, entry] : fonts) {
        for (auto& [size, font] : entry.sizeCache) {
            TTF_CloseFont(font);
        }
    }
    fonts.clear();
    currentFontId = 0;
    currentFont = nullptr;
}

TTF_Font* FontManager::getOrCreateFontAtSize(int fontId, float size) {
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
