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
    [womf.samplerType.quat] = function() return quat(0, 0, 0, 0) end,
}

local channelFinalizers = {
    [womf.samplerType.scalar] = function(v) return v end,
    [womf.samplerType.vec3] = function(v) return v end,
    [womf.samplerType.quat] = function(q) return q:normalize() end,
}
