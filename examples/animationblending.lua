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

scene.animations["Shoot"]:setLooping(false)

local runSpeed = 0.0
local mixer = womf.AnimationMixer {
    idle = scene.animations["Idle"],
    walk = scene.animations["Walk"],
    run = scene.animations["Run"],
    shoot = scene.animations["Shoot"],
}

mixer:setLayer("shoot", 2)
-- I just went through all the bones and took everything I might want. It looks okay.
mixer:setMask("shoot", {
    "Head", "Neck",
    "Pinky2.R", "Pinky1.R",
    "PalmP.R", "Ring2.R", "Ring1.R",
    "PalmR.R", "Index2.R", "Index1.R",
    "PalmI.R", "Thumb2.R", "Thumb1.R",
    "PalmT.R",
    "LowerArm.R", "UpperArm.R", "Shoulder.R"})
mixer:setWeight("shoot", 1.0)
mixer:setLayerAlpha(2, 0.0)

local function fadeInOut(time, duration, fadeIn, fadeOut)
    if time < fadeIn then
        return time / fadeIn
    elseif time < duration - fadeOut then
        return 1.0
    elseif time < duration then
        return (duration - time) / fadeOut
    end
    return 0.0
end

local function update(dt)
    -- up minus down
    local input = (womf.isDown(1073741906) and 1 or 0) - (womf.isDown(1073741905) and 1 or 0)
    if input ~= 0 then
        runSpeed = math.min(math.max(runSpeed + input * dt * 0.5, 0.0), 1.0)
        print(runSpeed)
    end

    if runSpeed < 0.5 then
        local t = runSpeed / 0.5
        mixer:setWeights({idle = 1 - t, walk = t, run = 0})
    elseif runSpeed >= 0.5 then
        local t = (runSpeed - 0.5) / 0.5
        mixer:setWeights({idle = 0, walk = 1 - t, run = t})
    end

    mixer:setLayerAlpha(2, fadeInOut(mixer:tell("shoot"), mixer.animations["shoot"].duration, 0.1, 0.1))

    local pose = mixer:update(dt)
    scene.skins[1]:pose(pose)
end

local function draw()
    womf.clear(0, 0, 0, 0, 1)
    scene:draw(shader, trafo)
    womf.present()
end

local function main()
    local time = womf.getTime()
    while true do
        for event in womf.pollEvent() do
            print("event", event.type)
            if event.type == "quit" then
                return
            elseif event.type == "windowresized" then
                print("window resized", event.width, event.height)
            elseif event.type == "keydown" then
                print(inspect(event))
                if event.symbol == 27 then
                    return
                elseif event.symbol == 43 then -- +
                    runSpeed = math.min(math.max(runSpeed + 0.1, 0.0), 1.0)
                    print(runSpeed)
                elseif event.symbol == 45 then -- -
                    runSpeed = math.min(math.max(runSpeed - 0.1, 0.0), 1.0)
                    print(runSpeed)
                elseif event.symbol == 105 then -- i
                    runSpeed = 0.0
                    print(runSpeed)
                elseif event.symbol == 119 then -- w
                    runSpeed = 0.5
                    print(runSpeed)
                elseif event.symbol == 114 then -- r
                    runSpeed = 1.0
                    print(runSpeed)
                elseif event.symbol == 32 then
                    mixer:stop("shoot")
                    mixer:play("shoot")
                end
            end
        end

        local now = womf.getTime()
        local dt = now - time
        time = now

        update(dt)
        draw()
    end
end

return main
