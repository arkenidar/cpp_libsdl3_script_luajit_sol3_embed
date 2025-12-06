-- Test script for Phase 3: TextWidget
---@diagnostic disable: undefined-global

print("=== Phase 3 Test: TextWidget ===")

-- Load font (required for TextWidget)
local font = loadFont("assets/DejaVuSans.ttf", 18)
if font then
    print("Font loaded successfully")
else
    print("ERROR: Could not load font - TextWidget requires a font!")
end

setWindowTitle("Phase 3 Test: TextWidget")

-- Create single-line text widget
local singleLine = createTextWidget({
    x = 50,
    y = 50,
    width = 400,
    height = 35,
    multiline = false,
    editable = true
})
singleLine:setText("Single-line text input")

-- Create multiline text widget
local multiLine = createTextWidget({
    x = 50,
    y = 120,
    width = 400,
    height = 150,
    multiline = true,
    editable = true
})
multiLine:setText("This is a multiline text widget.\nYou can type multiple lines here.\nUse arrow keys to navigate.\n\nTry selecting text with Shift+arrows or mouse drag!")

-- Create read-only text widget
local readOnly = createTextWidget({
    x = 50,
    y = 300,
    width = 400,
    height = 35,
    multiline = false,
    editable = false
})
readOnly:setText("Read-only text (cannot edit)")

function update(dt)
    singleLine:update(dt)
    multiLine:update(dt)
    readOnly:update(dt)
end

function render()
    local winSize = getWindowSize()

    setFontSize(20)
    drawText("Phase 3: TextWidget Demo", 20, 15, 1.0, 1.0, 0.5)

    setFontSize(14)

    -- Labels
    drawText("Single-line input:", 50, 35, 0.7, 0.7, 0.7)
    drawText("Multi-line input:", 50, 105, 0.7, 0.7, 0.7)
    drawText("Read-only:", 50, 285, 0.7, 0.7, 0.7)

    -- Render widgets
    singleLine:render()
    multiLine:render()
    readOnly:render()

    -- Instructions
    local y = 360
    drawText("Instructions:", 50, y, 0.6, 0.8, 0.6); y = y + 20
    drawText("- Click a field to focus and start typing", 50, y, 0.6, 0.6, 0.6); y = y + 18
    drawText("- Arrow keys: navigate cursor", 50, y, 0.6, 0.6, 0.6); y = y + 18
    drawText("- Shift+Arrow: select text", 50, y, 0.6, 0.6, 0.6); y = y + 18
    drawText("- Ctrl+A: select all", 50, y, 0.6, 0.6, 0.6); y = y + 18
    drawText("- Ctrl+C/X/V: copy/cut/paste", 50, y, 0.6, 0.6, 0.6); y = y + 18
    drawText("- Home/End: go to line start/end", 50, y, 0.6, 0.6, 0.6); y = y + 18
    drawText("- Click and drag to select with mouse", 50, y, 0.6, 0.6, 0.6); y = y + 18
    drawText("- Enter key inserts newline in multiline mode", 50, y, 0.6, 0.6, 0.6); y = y + 25

    -- Show current text content
    drawText("Current values:", 50, y, 0.6, 0.8, 0.6); y = y + 20
    drawText("Single: \"" .. singleLine:getText() .. "\"", 50, y, 0.5, 0.7, 0.5); y = y + 18

    -- Quit instruction
    setFontSize(12)
    drawText("Press ESC to quit", 20, winSize.height - 20, 0.5, 0.5, 0.5)
end

function onKeyDown(key)
    if key == "Escape" then
        quit()
    end
end

print("Phase 3 test script loaded. Click a text field to start editing!")
