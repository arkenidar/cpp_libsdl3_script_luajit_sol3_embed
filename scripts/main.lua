-- Main Lua script for SDL3 + LuaJIT + Sol3 demo

print("Lua script loaded successfully!")
print("Press ESC to quit, SPACE to change colors")

-- Initialize state
local time = 0
local colorIndex = 0
local colors = {
    {0.1, 0.1, 0.15},
    {0.15, 0.1, 0.2},
    {0.1, 0.15, 0.15},
    {0.2, 0.1, 0.1}
}

-- Set initial window title
setWindowTitle("SDL3 + LuaJIT + Sol3 Demo")

-- Update function called every frame
function update(deltaTime)
    time = time + deltaTime
end

-- Render function called every frame
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
end

-- Key down event handler
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

print("Lua initialization complete. Enjoy the demo!")
