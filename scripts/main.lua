-- Main Lua script for SDL3 + LuaJIT + Sol3 demo

-- C++ API functions (injected by Sol3 at runtime)
---@diagnostic disable: undefined-global

print("Lua script loaded successfully!")
print("Press ESC to quit, SPACE to change colors")
print("Click/touch the button to trigger an action")

-- Initialize state
local time = 0
local colorIndex = 0
local colors = {
    {0.1, 0.1, 0.15},
    {0.15, 0.1, 0.2},
    {0.1, 0.15, 0.15},
    {0.2, 0.1, 0.1}
}

-- Button state
local button = {
    x = 50,
    y = 50,
    width = 150,
    height = 50,
    clicked = false,
    clickTime = 0
}

-- Set initial window title
setWindowTitle("SDL3 + LuaJIT + Sol3 Demo")

-- Update function called every frame (global for C++ callback)
---@diagnostic disable-next-line: lowercase-global
function update(deltaTime)
    time = time + deltaTime
    -- Reset button click visual after 0.2 seconds
    if button.clicked and (time - button.clickTime) > 0.2 then
        button.clicked = false
    end
end

-- Render function called every frame (global for C++ callback)
---@diagnostic disable-next-line: lowercase-global
function render()
    local winSize = getWindowSize()
    local centerX = winSize.width / 2
    local centerY = winSize.height / 2

    -- Draw animated rectangles
    for i = 1, 5 do
        local offset = (i - 1) * 40
        local x = centerX - 100 + math.sin(time + i) * 50
        local y = centerY - 100 + offset
        local size = 80 + math.sin(time * 2 + i) * 20

        local r = (i / 5) * 0.8 + 0.2
        local g = 0.3 + math.sin(time + i) * 0.3
        local b = 0.5 + math.cos(time + i) * 0.3

        drawRect(x, y, size, size, r, g, b, 0.8)
    end

    -- Draw center square
    local size = 100 + math.sin(time * 2) * 30
    drawRect(centerX - size/2, centerY - size/2, size, size, 1.0, 0.8, 0.2, 0.9)

    -- Draw clickable button
    local btnR, btnG, btnB = 0.2, 0.6, 0.3
    if button.clicked then
        btnR, btnG, btnB = 0.4, 0.9, 0.5
    end
    drawRect(button.x, button.y, button.width, button.height, btnR, btnG, btnB, 1.0)
end

-- Key down event handler (global for C++ callback)
---@diagnostic disable-next-line: lowercase-global
function onKeyDown(key)
    if key == "Escape" then
        print("Quitting application...")
        quit()
    elseif key == "Space" then
        colorIndex = (colorIndex + 1) % #colors
        local color = colors[colorIndex + 1]
        setBackgroundColor(color[1], color[2], color[3])
        print("Background color changed")
    end
end

-- Mouse/touch down event handler (global for C++ callback)
---@diagnostic disable-next-line: lowercase-global
function onMouseDown(x, y, buttonId)
    -- Check if click is inside the button
    if x >= button.x and x <= button.x + button.width and
       y >= button.y and y <= button.y + button.height then
        button.clicked = true
        button.clickTime = time
        print("Button clicked!")
    end
end

print("Lua initialization complete. Enjoy the demo!")
