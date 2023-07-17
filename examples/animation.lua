local shader = womf.Shader("assets/skinning.vert", "assets/default.frag")

local scene = womf.loadGltf("assets/Mike.gltf")

local xRes, yRes = womf.getWindowSize()
womf.setProjectionMatrix(45, xRes/yRes, 0.1, 100.0)

local camTrafo = womf.Transform()
camTrafo:setPosition(0, 0, -7.0)
camTrafo:lookAt(0, 0, 0)
womf.setViewMatrix(camTrafo)

local trafo = womf.Transform()
trafo:setPosition(0, -2.5, 0)
trafo:rotateLocal(quat.from_angle_axis(math.pi, 0, 1, 0):unpack())

local animations = {
    scene.animations["Dance"],
    scene.animations["Hello"],
    scene.animations["Jump"],
    scene.animations["Run"],
}
local animationIndex = 1

local function main()
    local time = womf.getTime()
    while true do
        for event in womf.pollEvent() do
            if event.type == "quit" then
                return
            elseif event.type == "keydown" then
                if event.symbol == "escape" then
                    return
                elseif event.symbol == "space" then
                    animationIndex = (animationIndex % #animations) + 1
                end
            end
        end

        local now = womf.getTime()
        local dt = now - time
        time = now

        local pose = animations[animationIndex]:update(dt)
        scene.skins[1]:pose(pose)

        womf.clear(0, 0, 0, 0, 1)
        scene:draw(shader, trafo)
        womf.present()
    end
end

return main
