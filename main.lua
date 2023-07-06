local tex = womf.Texture("assets/test.png")

local shader = womf.Shader("assets/default.vert", "assets/default.frag")

local attr = womf.GraphicsBuffer(womf.bufferTarget.attributes, womf.bufferUsage.static, "assets/quad.attributes.bin")
local idx = womf.GraphicsBuffer(womf.bufferTarget.indices, womf.bufferUsage.static, "assets/quad.indices.bin")

local fmt = womf.VertexFormat {
    { "position", womf.attrType.f32, 3 },
    { "texcoord0", womf.attrType.f32, 2 },
}

local geom = womf.Geometry(womf.drawMode.triangles, fmt, attr, womf.attrType.u16, idx)

local xRes, yRes = womf.getWindowSize()
womf.setProjection(45, xRes/yRes, 0.1, 100.0)

local camTrafo = womf.Transform()
camTrafo:lookAt(0, 0, 1)
womf.setView(camTrafo)

local trafo = womf.Transform()
trafo:setPosition(0, 0, 2.0)

local function draw()
    womf.clear(0, 0, 0, 0, 1)
    womf.draw(shader, geom, trafo, {texture = tex})
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
