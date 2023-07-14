womf.Animation = class("Animation")

local bufferTypeMap = {
    [womf.samplerType.scalar] = "f32",
    [womf.samplerType.vec3] = "vec3",
    [womf.samplerType.quat] = "vec4",
}

local samplerTypeMap = {
    [womf.samplerType.scalar] = function(v) return v end,
    [womf.samplerType.vec3] = vec3,
    [womf.samplerType.quat] = quat,
}

function womf.Animation:initialize()
    self.channels = {}
    self.state = {}
    self.duration = 0
    self.looping = true
    self.time = 0
end

function womf.Animation:setLooping(looping)
    self.looping = looping
end

-- times must be monotonically increasing scalars >= 0
function womf.Animation:addChannel(key, samplerType, interp, times, values)
    interp = interp or womf.interp.linear
    assert(interp == womf.interp.step or interp == womf.interp.linear)
    if type(times) == "table" then
        times = womf.Buffer("f32", times)
    end
    if type(values) == "table" then
        values = womf.Buffer(bufferTypeMap[samplerType], values)
    end
    local channel = {
        key = key,
        interp = interp,
        samplerType = samplerType,
        sampler = womf.Sampler(samplerType, interp, times, values)
    }
    self.channels[key] = channel
    self.duration = math.max(self.duration, channel.sampler:getDuration())
    self.state[key] = self:sample(key, self.time)
end

function womf.Animation:getState()
    return self.state
end

function womf.Animation:sample(key, time)
    local channel = self.channels[key]
    return samplerTypeMap[channel.samplerType](channel.sampler:sample(time))
end

function womf.Animation:seek(time)
    if self.looping then
        time = time % self.duration
    end
    self.time = time
    for key, channel in pairs(self.channels) do
        self.state[key] = self:sample(key, time)
    end
    return self.state
end

function womf.Animation:update(dt)
    return self:seek(self.time + dt)
end

function womf.Animation:getDuration()
    return self.duration
end

local channelConstructors = {
    [womf.samplerType.scalar] = function() return 0.0 end,
    [womf.samplerType.vec3] = function() return vec3(0, 0, 0) end,
    [womf.samplerType.quat] = function() return quat(0, 0, 0, 1) end,
}

local channelFinalizers = {
    [womf.samplerType.scalar] = function(v) return v end,
    [womf.samplerType.vec3] = function(v) return v end,
    [womf.samplerType.quat] = function(q) return q:normalize() end,
}

womf.AnimationMixer = class("AnimationMixer")

function womf.AnimationMixer:initialize(animations)
    assert(next(animations) ~= nil) -- not empty
    self.layers = {
        {animations = {}, alpha = 1.0, state = {}},
    }
    -- self.layers might be sparse, so we need to keep track of the indices (sorted)
    self.layerIndices = {1}
    self.animations = animations
    self.animationData = {}
    for name, animation in pairs(animations) do
        -- Maybe add `range` to this that specifies start and end point
        -- Maybe add `loopPoint`
        self.animationData[name] = {
            weight = 0.0,
            weightSpeed = 0.0,
            targetWeight = 0.0,
            speed = 1.0,
            mask = nil,
            time = 0.0,
            layer = 1,
            -- Play all looping animations, so they stay in sync
            playing = animation.looping, -- Whether to advance the time
            callbacks = {},
            duration = animation:getDuration(),
        }
        self.layers[1].animations[name] = self.animationData[name]
        -- just do it for the first animation
        if not self.channels then
            self.channels = {}
            for key, channel in pairs(animation.channels) do
                self.channels[key] = {
                    constructor = channelConstructors[channel.samplerType],
                    finalizer = channelFinalizers[channel.samplerType],
                }
            end
        end
    end
    self.state = {}
end

function womf.AnimationMixer:setLayer(name, layer)
    if self.layers[layer] == nil then
        self.layers[layer] = {animations = {}, alpha = 1.0, state = {}}
    end

    local anim = self.animationData[name]
    self.layers[anim.layer].animations[name] = nil
    self.layers[layer].animations[name] = anim
    anim.layer = layer

    -- update layerIndices
    local layerSet = {}
    for _, anim in pairs(self.animationData) do
        layerSet[anim.layer] = true
    end

    self.layerIndices = {}
    for layerIdx, _ in pairs(layerSet) do
        table.insert(self.layerIndices, layerIdx)
    end
    table.sort(self.layerIndices)
end

function womf.AnimationMixer:setLayerAlpha(layer, alpha)
    if self.layers[layer] then
        self.layers[layer].alpha = alpha
    else
        self.layers[layer] = {animations = {}, alpha = alpha, state = {}}
    end
end

function womf.AnimationMixer:play(name)
    self.animationData[name].playing = true
end

function womf.AnimationMixer:pause(name)
    self.animationData[name].playing = false
end

function womf.AnimationMixer:stop(name)
    self.animationData[name].playing = false
    self.animationData[name].time = 0
end

function womf.AnimationMixer:isPlaying(name)
    return self.animationData[name].playing
end

function womf.AnimationMixer:isFinished(name)
    return not self.animationData[name].playing and self.animationData[name].time >= self.animationData[name].duration
end

function womf.AnimationMixer:setMask(name, mask)
    if mask == nil then
        self.animationData[name].mask = nil
    else
        self.animationData[name].mask = {}
        for _, elem in ipairs(mask) do
            local prefix = elem .. "/"
            for key, _ in pairs(self.channels) do
                if key:sub(1, #prefix) == prefix then
                    self.animationData[name].mask[key] = true
                end
            end
        end
    end
end

function womf.AnimationMixer:setWeight(name, weight)
    self.animationData[name].weight = weight
    self.animationData[name].weightVelocity = 0.0
end

-- works with mixer:setWeights(blendSpace:getWeights({x, y}))
function womf.AnimationMixer:setWeights(weights)
    for name, weight in pairs(weights) do
        self:setWeight(name, weight)
    end
end

function womf.AnimationMixer:seek(name, time)
    self.animationData[name].time = time
end

function womf.AnimationMixer:tell(name)
    return self.animationData[name].time
end

local function crossed(before, thresh, after)
    local deltaBefore = thresh - before
    local deltaAfter = thresh - after
    -- this is 0 if one of the deltas is zero and negative if the signs of deltaBefore and
    -- deltaAfter are different
    return deltaBefore * deltaAfter <= 0
end

local function mixStateValue(a, aWeight, b, bWeight)
    if quat.is_quat(a) then
        -- q and -q represent the same rotation, so if I try to blend two quaternions
        -- which are close together, but differ in sign, I get really small values
        -- and after normalization, nothing sensible is left.
        -- You might think this is rare, but it happens blending almost any two animations!
        -- To avoid this, I check if the quats point away from each other and if so, I flip one of them.
        -- I use `>=` so it doesn't flip the sign for the first animation being added (oldValue is 0,0,0,0).
        local sign = a:dot(b) >= 0 and 1 or -1
        return a * aWeight + b * (bWeight * sign)
    else
        return a * aWeight + b * bWeight
    end
end


function womf.AnimationMixer:_finalizeState(state)
    for key, value in pairs(state) do
        state[key] = self.channels[key].finalizer(value)
    end
end

function womf.AnimationMixer:update(dt)
    -- update animations
    for name, anim in pairs(self.animationData) do
        if anim.weightSpeed ~= 0.0 then
            local weightBefore = anim.weight
            local sign = anim.targetWeight - anim.weight > 0 and 1 or -1
            anim.weight = anim.weight + anim.weightSpeed * sign * dt
            if crossed(weightBefore, anim.targetWeight, anim.weight) then
                anim.weight = anim.targetWeight
                anim.weightSpeed = 0.0
            end
        end

        if anim.playing then
            local timeBefore = anim.time
            anim.time = anim.time + dt * anim.speed

            for _, cb in ipairs(anim.callbacks) do
                if crossed(timeBefore, cb.time, anim.time) then
                    cb.func(self, name)
                end
            end

            if not self.animations[name].looping and anim.time >= anim.duration then
                anim.playing = false
                anim.time = 0.0
            end
        end

        if anim.weight > 0.0 then
            anim.state = self.animations[name]:seek(anim.time)
        end
    end

    -- build layer states
    -- animations in a single layer are blended order-independently
    for _, layerIdx in ipairs(self.layerIndices) do
        local layer = self.layers[layerIdx]

        -- Just leave out keys if they are not part of a layer
        layer.state = {}
        for name, anim in pairs(layer.animations) do
            if anim.weight > 0.0 then
                for key, value in pairs(anim.state) do
                    if not anim.mask or anim.mask[key] then
                        if layer.state[key] then
                            layer.state[key] = mixStateValue(layer.state[key], 1.0, value, anim.weight)
                        else
                            layer.state[key] = value * anim.weight
                        end
                    end
                end
            end
        end

        self:_finalizeState(layer.state)
    end

    -- combine layers
    -- this is order-dependent as a series of linear interpolations between layers
    self.state = {}
    for _, layerIdx in ipairs(self.layerIndices) do
        local layer = self.layers[layerIdx]
        for key, value in pairs(layer.state) do
            if self.state[key] then
                self.state[key] = mixStateValue(self.state[key], (1 - layer.alpha), value, layer.alpha)
            else
                -- set regardless of alpha
                self.state[key] = value
            end
        end
    end

    -- Here we need to make sure we provide values for all channels
    for key, channel in pairs(self.channels) do
        if not self.state[key] then
            self.state[key] = channel.constructor()
        end
    end

    self:_finalizeState(self.state)

    return self.state
end

function womf.AnimationMixer:setSpeed(name, speed)
    self.animationData[name].speed = speed
end

function womf.AnimationMixer:setAnimation(name)
    for _, anim in pairs(self.animationData) do
        anim.weight = 0.0
        anim.weightVelocity = 0.0
    end
    self.animationData[name].time = 0.0
    self.animationData[name].weight = 1.0
end

-- TODO: add a way to remove callbacks later
function womf.AnimationMixer:addCallback(name, time, func)
    table.insert(self.animationData[name].callbacks, {time = time, func = func})
end

function womf.AnimationMixer:fade(name, duration, targetWeight)
    if duration == 0.0 then
        self.animationData[name].weightSpeed = 0.0
        self.animationData[name].weight = targetWeight
        self.animationData[name].targetWeight = targetWeight
    else
        -- 1/duration, instead of delta/duration to be consistent!
        self.animationData[name].weightSpeed = 1.0 / duration
        self.animationData[name].targetWeight = targetWeight
    end
end

-- Maybe this should not be a separate function, because I cannot imagine a case other than
-- fadeInEx where this is useful
function womf.AnimationMixer:fadeAll(duration, targetWeight)
    for name, _ in pairs(self.animations) do
        self:fade(name, targetWeight, duration)
    end
end

function womf.AnimationMixer:fadeIn(name, duration, targetWeight)
    self:fade(name, duration, targetWeight or 1.0)
end

function womf.AnimationMixer:fadeOut(name, duration)
    self:fade(name, duration, targetWeight or 0.0)
end

function womf.AnimationMixer:fadeInEx(name, duration)
    self:fadeAll(0.0, duration)
    self:fadeIn(name, duration)
end
