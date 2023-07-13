local gltf = require "gltf"

local tex = womf.Texture("assets/test.png")

local shader = womf.Shader("assets/default.vert", "assets/default.frag")

local attr = womf.GraphicsBuffer(womf.bufferTarget.attributes, womf.bufferUsage.static, "assets/quad.attributes.bin")
local idx = womf.GraphicsBuffer(womf.bufferTarget.indices, womf.bufferUsage.static, "assets/quad.indices.bin")

local fmt = womf.VertexFormat {
    { "position", womf.attrType.f32, 3 },
    { "texcoord0", womf.attrType.f32, 2 },
}

local geom = womf.Geometry(womf.drawMode.triangles)
geom:addVertexBuffer(fmt, attr)
geom:setIndexBuffer(womf.attrType.u16, idx)

local scene = womf.loadGltf("assets/Avocado.gltf")

local xRes, yRes = womf.getWindowSize()
womf.setProjectionMatrix(45, xRes/yRes, 0.1, 100.0)

local camTrafo = womf.Transform()
camTrafo:setPosition(0, 0, -0.15)
camTrafo:lookAt(0, 0, 0)
womf.setViewMatrix(camTrafo)

local trafo = womf.Transform()
trafo:rotate(2.0 * math.acos(math.pi * 0.5), 0, 1, 0)

local function draw()
    womf.clear(0, 0, 0, 0, 1)
    -- womf.setModelMatrix(trafo)
    -- womf.draw(shader, geom, {texture = tex})
    scene:draw(shader)
    womf.present()
end

local function main()
    local time = womf.getTime()
    while true do
        for event in womf.pollEvent() do
            print("event", event.type)
            if event.type == "quit" then
                return
            elseif event.type == "keydown" and event.symbol == 27 then
                return
            elseif event.type == "windowresized" then
                print("window resized", event.width, event.height)
            end
        end

        local now = womf.getTime()
        local dt = now - time
        time = now

        draw()
    end
end

return main
