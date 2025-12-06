-- Test script for Phase 2: Text Widget Primitives
---@diagnostic disable: undefined-global

print("=== Phase 2 Test Script ===")

-- Load font
local font = loadFont("assets/DejaVuSans.ttf", 24)
if font then
    print("Font loaded successfully")
else
    print("ERROR: Could not load font")
end

setWindowTitle("Phase 2 Test: Text Widget Primitives")

-- Test state
local inputText = "Hello, World!"
local cursorPos = #inputText
local cursorBlink = 0
local textInputEnabled = false
local mousePos = {x = 0, y = 0}
local lastMouseButton = "none"
local lastKey = ""
local lastTextInput = ""
local scrollAmount = 0

-- Text field position
local textField = {x = 50, y = 100, w = 400, h = 40}

function update(dt)
    cursorBlink = cursorBlink + dt
    if cursorBlink > 1.0 then
        cursorBlink = cursorBlink - 1.0
    end
end

function render()
    local winSize = getWindowSize()
    setFontSize(20)

    -- Title
    drawText("Phase 2 Feature Test", 20, 20, 1.0, 1.0, 0.5)

    -- Instructions
    setFontSize(14)
    drawText("Click the text field to enable text input. Type to test onTextInput.", 20, 50, 0.8, 0.8, 0.8)
    drawText("Press Ctrl+C to copy, Ctrl+V to paste. Use arrow keys, Home, End.", 20, 70, 0.8, 0.8, 0.8)

    -- Draw text field background
    drawRect(textField.x, textField.y, textField.w, textField.h, 0.2, 0.2, 0.25, 1.0)

    -- Draw text field outline (active state)
    if textInputEnabled then
        drawRectOutline(textField.x, textField.y, textField.w, textField.h, 0.3, 0.6, 1.0, 1.0)
    else
        drawRectOutline(textField.x, textField.y, textField.w, textField.h, 0.4, 0.4, 0.5, 1.0)
    end

    -- Draw text inside field
    setFontSize(20)
    drawText(inputText, textField.x + 10, textField.y + 10, 1.0, 1.0, 1.0)

    -- Draw cursor if text input enabled
    if textInputEnabled and cursorBlink < 0.5 then
        local cursorX = textField.x + 10 + measureTextToOffset(inputText, cursorPos)
        drawLine(cursorX, textField.y + 8, cursorX, textField.y + textField.h - 8, 1.0, 1.0, 1.0, 1.0)
    end

    -- Debug info section
    setFontSize(14)
    local y = 180
    local lineHeight = 20

    drawText("--- Debug Info ---", 20, y, 0.6, 0.8, 0.6); y = y + lineHeight
    drawText("Text input active: " .. tostring(textInputEnabled), 20, y, 0.7, 0.7, 0.7); y = y + lineHeight
    drawText("Input text: \"" .. inputText .. "\"", 20, y, 0.7, 0.7, 0.7); y = y + lineHeight
    drawText("Cursor position: " .. cursorPos .. " / " .. #inputText, 20, y, 0.7, 0.7, 0.7); y = y + lineHeight
    drawText("Last key: " .. lastKey, 20, y, 0.7, 0.7, 0.7); y = y + lineHeight
    drawText("Last text input: \"" .. lastTextInput .. "\"", 20, y, 0.7, 0.7, 0.7); y = y + lineHeight
    drawText("Mouse pos: " .. string.format("%.0f, %.0f", mousePos.x, mousePos.y), 20, y, 0.7, 0.7, 0.7); y = y + lineHeight
    drawText("Last mouse button: " .. lastMouseButton, 20, y, 0.7, 0.7, 0.7); y = y + lineHeight
    drawText("Scroll amount: " .. string.format("%.1f", scrollAmount), 20, y, 0.7, 0.7, 0.7); y = y + lineHeight

    -- Key modifiers
    local mods = getKeyModifiers()
    local modStr = ""
    if mods.shift then modStr = modStr .. "SHIFT " end
    if mods.ctrl then modStr = modStr .. "CTRL " end
    if mods.alt then modStr = modStr .. "ALT " end
    if mods.gui then modStr = modStr .. "GUI " end
    if modStr == "" then modStr = "none" end
    drawText("Modifiers: " .. modStr, 20, y, 0.7, 0.7, 0.7); y = y + lineHeight

    -- Clipboard status
    drawText("Has clipboard: " .. tostring(hasClipboardText()), 20, y, 0.7, 0.7, 0.7); y = y + lineHeight

    -- Test measureTextToOffset and getOffsetFromX
    y = y + lineHeight
    drawText("--- Text Measurement Tests ---", 20, y, 0.6, 0.8, 0.6); y = y + lineHeight
    local testText = "Hello World"
    local width5 = measureTextToOffset(testText, 5)
    local offset100 = getOffsetFromX(testText, 100)
    drawText("measureTextToOffset(\"" .. testText .. "\", 5) = " .. width5, 20, y, 0.7, 0.7, 0.7); y = y + lineHeight
    drawText("getOffsetFromX(\"" .. testText .. "\", 100) = " .. offset100, 20, y, 0.7, 0.7, 0.7); y = y + lineHeight

    -- Draw lines demo
    y = y + lineHeight
    drawText("--- Drawing Tests ---", 20, y, 0.6, 0.8, 0.6); y = y + lineHeight
    drawLine(20, y + 10, 200, y + 10, 1.0, 0.5, 0.5, 1.0)
    drawLine(20, y + 20, 200, y + 30, 0.5, 1.0, 0.5, 1.0)
    drawRectOutline(220, y, 100, 40, 0.5, 0.5, 1.0, 1.0)

    -- Quit instruction
    setFontSize(12)
    drawText("Press ESC to quit", 20, winSize.height - 20, 0.5, 0.5, 0.5)
end

function onKeyDown(key)
    lastKey = key
    local mods = getKeyModifiers()

    if key == "Escape" then
        quit()
    elseif textInputEnabled then
        if key == "Backspace" and cursorPos > 0 then
            inputText = inputText:sub(1, cursorPos - 1) .. inputText:sub(cursorPos + 1)
            cursorPos = cursorPos - 1
        elseif key == "Delete" and cursorPos < #inputText then
            inputText = inputText:sub(1, cursorPos) .. inputText:sub(cursorPos + 2)
        elseif key == "Left" then
            if cursorPos > 0 then cursorPos = cursorPos - 1 end
        elseif key == "Right" then
            if cursorPos < #inputText then cursorPos = cursorPos + 1 end
        elseif key == "Home" then
            cursorPos = 0
        elseif key == "End" then
            cursorPos = #inputText
        elseif mods.ctrl and (key == "C" or key == "c") then
            setClipboardText(inputText)
            print("Copied to clipboard: " .. inputText)
        elseif mods.ctrl and (key == "V" or key == "v") then
            local clip = getClipboardText()
            if clip then
                inputText = inputText:sub(1, cursorPos) .. clip .. inputText:sub(cursorPos + 1)
                cursorPos = cursorPos + #clip
                print("Pasted from clipboard: " .. clip)
            end
        end
    end
end

function onKeyUp(key)
    -- Just log it
end

function onTextInput(text)
    lastTextInput = text
    if textInputEnabled then
        inputText = inputText:sub(1, cursorPos) .. text .. inputText:sub(cursorPos + 1)
        cursorPos = cursorPos + #text
    end
end

function onMouseDown(x, y, button)
    lastMouseButton = "down:" .. button

    -- Check if clicked on text field
    if x >= textField.x and x <= textField.x + textField.w and
       y >= textField.y and y <= textField.y + textField.h then
        if not textInputEnabled then
            textInputEnabled = true
            startTextInput()
            setTextInputArea(textField.x, textField.y, textField.w, textField.h, 0)
            print("Text input started")
        end
        -- Position cursor based on click
        local relX = x - textField.x - 10
        cursorPos = getOffsetFromX(inputText, relX)
    else
        -- Clicked outside, stop text input
        if textInputEnabled then
            textInputEnabled = false
            stopTextInput()
            print("Text input stopped")
        end
    end
end

function onMouseUp(x, y, button)
    lastMouseButton = "up:" .. button
end

function onMouseMove(x, y)
    mousePos.x = x
    mousePos.y = y
end

function onMouseWheel(x, y, scrollX, scrollY)
    scrollAmount = scrollAmount + scrollY
end

function onTouchDown(fingerId, x, y, pressure)
    print(string.format("Touch down: finger=%d, pos=(%.0f, %.0f), pressure=%.2f", fingerId, x, y, pressure))
end

function onTouchUp(fingerId, x, y)
    print(string.format("Touch up: finger=%d, pos=(%.0f, %.0f)", fingerId, x, y))
end

function onTouchMove(fingerId, x, y, dx, dy)
    -- Could log but might be too noisy
end

print("Phase 2 test script loaded. Click the text field to test input!")
