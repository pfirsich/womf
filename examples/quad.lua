local tex = womf.Texture("assets/test.png")

local shader = womf.Shader("assets/default.vert", "assets/default.frag")

local attr = womf.GraphicsBuffer(womf.bufferTarget.attributes, womf.bufferUsage.static, "assets/quad.attributes.bin")
local idx = womf.GraphicsBuffer(womf.bufferTarget.indices, womf.bufferUsage.static, "assets/quad.indices.bin")

local fmt = womf.VertexFormat {
    { "position", womf.attrType.f32, 3 },
    { "texcoord0", womf.attrType.f32, 2 },
}

local quad = womf.Geometry(womf.drawMode.triangles)
quad:addVertexBuffer(fmt, attr)
quad:setIndexBuffer(womf.attrType.u16, idx)

local xRes, yRes = womf.getWindowSize()
womf.setProjectionMatrix(45, xRes/yRes, 0.1, 100.0)

local camTrafo = womf.Transform()
camTrafo:setPosition(0, 0, -2)
camTrafo:lookAt(0, 0, 0)
womf.setViewMatrix(camTrafo)

local trafo = womf.Transform()

local function main()
    local time = womf.getTime()
    while true do
        for event in womf.pollEvent() do
            if event.type == "quit" then
                return
            elseif event.type == "keydown" and event.symbol == 27 then
                return
            end
        end

        womf.clear(0, 0, 0, 0, 1)
        womf.setModelMatrix(trafo)
        womf.draw(shader, quad, {texture = tex, color = {1, 1, 1, 1}})
        womf.present()
    end
end

return main
