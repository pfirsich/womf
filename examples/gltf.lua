local shader = womf.Shader("assets/default.vert", "assets/default.frag")

local scene = womf.loadGltf("assets/Avocado.gltf")

local xRes, yRes = womf.getWindowSize()
womf.setProjectionMatrix(45, xRes/yRes, 0.1, 100.0)

local camTrafo = womf.Transform()
camTrafo:setPosition(0, 0, -0.1)
camTrafo:lookAt(0, 0, 0)
womf.setViewMatrix(camTrafo)

local trafo = womf.Transform()
trafo:setPosition(0, -0.025, 0.0)

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
        scene:draw(shader, trafo)
        womf.present()
    end
end

return main
