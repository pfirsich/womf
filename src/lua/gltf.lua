local json = require "json"

local pixelTexture = womf.pixelTexture(1, 1, 1, 1)

local function walkNode(node, func)
    func(node)
    for _, child in ipairs(node.children) do
        walkNode(child, func)
    end
end

local function walkScene(scene, func)
    for _, node in ipairs(scene) do
        walkNode(node, func)
    end
end

local function getGlobalTransform(node, rootTrafo)
    rootTrafo = rootTrafo or mat4()
    local parentTrafo = node.parent and node.parent.fullTransform or rootTrafo
    local nodeTrafo = mat4(node.transform:getMatrix())
    node.fullTransform = parentTrafo * nodeTrafo
    return node.fullTransform
end

local function drawScene(scene, shader, sceneTransform)
    sceneTransform = sceneTransform and mat4(sceneTransform:getMatrix()) or mat4()
    walkScene(scene, function(node)
        if node.mesh then
            womf.setModelMatrix(getGlobalTransform(node, sceneTransform):unpack())
            for _, prim in ipairs(node.mesh.primitives) do
                womf.draw(shader, prim.geometry, {
                    jointMatrices = node.skin and node.skin.jointMatrices,
                    texture = prim.material.albedo or pixelTexture,
                    color = prim.material.color,
                })
            end
        end
    end)
end

local function makeMat4(src)
    local m = mat4()
    for i = 1, 16 do
        m[i] = src[i - 1]
    end
    return m
end

local function updateSkin(skin)
    local invGlobalTransform = mat4.invert(mat4(), getGlobalTransform(skin.rootNode))
    for i, joint in ipairs(skin.joints) do
        local parentTrafo = joint.node.parent and joint.node.parent.fullTransform or mat4()
        joint.node.fullTransform = parentTrafo * mat4(joint.node.transform:getMatrix())
        skin.jointMatrices[i] = {(invGlobalTransform * joint.node.fullTransform * joint.inverseBindMatrix):unpack()}
    end
end

local function poseSkin(skin, pose)
    for key, value in pairs(pose) do
        local bone, component = key:match("([^/]+)/(.+)")
        local trafo = skin.joints[bone].node.transform
        if component == "translation" then
            trafo:setPosition(value:unpack())
        elseif component == "rotation" then
            trafo:setOrientation(value:unpack())
        end
    end
    skin:update()
end

function womf.loadGltf(filename)
    local data = json.decode(womf.readFile(filename))
    assert(#data.scenes == 1)

    local dir = filename:match("(.-/)[^/]+$") or "./"

    local ret = {}

    ret.buffers = {}
    for bufIdx, buffer in ipairs(data.buffers) do
        assert(buffer.uri)
        ret.buffers[bufIdx] = womf.Buffer(dir .. buffer.uri)
    end

    ret.bufferViews = {}
    for bvIdx, bv in ipairs(data.bufferViews) do
        ret.bufferViews[bvIdx] = womf.BufferView(ret.buffers[bv.buffer + 1], bv.byteOffset or 0, bv.byteLength)
    end

    ret.textures = {}
    for texIdx, texture in ipairs(data.textures or {}) do
        local image = data.images[texture.source + 1]
        if image.uri then
            ret.textures[texIdx] = womf.Texture(dir .. image.uri)
        elseif image.bufferView then
            ret.textures[texIdx] = womf.Texture(ret.bufferViews[image.bufferView + 1])
        else
            assert(image.uri or image.bufferView)
        end
    end

    -- materials
    ret.materials = {}
    for matIdx, mat in ipairs(data.materials) do
        assert(mat.pbrMetallicRoughness)
        ret.materials[matIdx] = {
            albedo = mat.pbrMetallicRoughness.baseColorTexture
                and ret.textures[mat.pbrMetallicRoughness.baseColorTexture.index + 1],
            color = mat.pbrMetallicRoughness.baseColorFactor or {1, 1, 1, 1},
            metallicRoughness = mat.pbrMetallicRoughness.metallicRoughnessTexture and
                ret.textures[mat.pbrMetallicRoughness.metallicRoughnessTexture.index + 1],
            metallicFactor = mat.pbrMetallicRoughness.metallicFactor or 1.0,
            roughnessFactor = mat.pbrMetallicRoughness.roughnessFactor or 1.0,
            normal = mat.normalTexture and ret.textures[mat.normalTexture.index + 1],
            occlusion = mat.occlusionTexture and ret.textures[mat.occlusionTexture.index + 1],
            name = mat.name,
        }
    end

    local attributeMap = {
        POSITION = "position",
        NORMAL = "normal",
        TANGENT = "tangent",
        TEXCOORD_0 = "texcoord0",
        TEXCOORD_1 = "texcoord1",
        COLOR_0 = "color0",
        COLOR_1 = "color1",
        JOINTS_0 = "joints0",
        WEIGHTS_0 = "weights0",
    }

    local componentMap = {
        SCALAR = 1,
        VEC2 = 2,
        VEC3 = 3,
        VEC4 = 4,
        MAT2 = 4,
        MAT3 = 9,
        MAT4 = 16,
    }

    local typeMap = {
        -- 5120 = i8,
        [5121] = womf.attrType.u8,
        -- 5122 = i16,
        [5123] = womf.attrType.u16,
        [5125] = womf.attrType.u32,
        [5126] = womf.attrType.f32,
    }
    local indexBuffers = {}

    ret.meshes = {}
    for meshIdx, mesh in ipairs(data.meshes) do
        ret.meshes[meshIdx] = {
            name = mesh.name,
            primitives = {},
        }

        for primIdx, prim in ipairs(mesh.primitives) do
            assert(prim.mode == nil or prim.mode == 4) -- triangles
            assert(prim.indices) -- indexed mesh
            assert(prim.material)

            local referencedBufferViews = {}
            for name, accessorIdx in ipairs(prim.attributes) do
                table.insert(referencedBufferViews, data.accessors[accessorIdx + 1].bufferView)
            end

            local vertexFormats = {}
            -- TODO: Use accessor.count to determine vertex count and set vertex range accordingly
            for attrName, accessorIdx in pairs(prim.attributes) do
                local accessor = data.accessors[accessorIdx + 1]
                local bufferView = ret.bufferViews[accessor.bufferView + 1]
                if not vertexFormats[bufferView] then
                    vertexFormats[bufferView] = {}
                end
                assert(not accessor.normalized)
                assert(accessor.byteOffset == nil or accessor.byteOffset == 0)
                local attrType = typeMap[accessor.componentType]
                assert(attrType)
                local count = componentMap[accessor.type]
                assert(count)
                table.insert(vertexFormats[bufferView], {attributeMap[attrName], attrType, count})
            end

            local geometry = womf.Geometry(womf.drawMode.triangles)

            for bufferView, vertexFormat in pairs(vertexFormats) do
                local vfmt = womf.VertexFormat(vertexFormat)
                local gbuf = womf.GraphicsBuffer(womf.bufferTarget.attributes, womf.bufferUsage.static, bufferView)
                geometry:addVertexBuffer(vfmt, gbuf)
            end

            if prim.indices then
                -- lets assume these are not reused
                local accessor = data.accessors[prim.indices + 1]
                local bufferView = ret.bufferViews[accessor.bufferView + 1]
                if not indexBuffers[bufferView] then
                    indexBuffers[bufferView] = womf.GraphicsBuffer(
                        womf.bufferTarget.indices, womf.bufferUsage.static, bufferView)
                end
                local indexType = typeMap[accessor.componentType]
                assert(indexType)
                geometry:setIndexBuffer(indexType, indexBuffers[bufferView])
            end

            ret.meshes[meshIdx].primitives[primIdx] = {
                geometry = geometry,
                material = ret.materials[prim.material + 1],
            }
        end
    end

    ret.skins = {}
    for skinIdx, skin in ipairs(data.skins or {}) do
        ret.skins[skinIdx] = {
            jointMatrices = {},
            update = updateSkin,
            pose = poseSkin,
        }
    end

    ret.nodes = {}
    for nodeIdx, node in ipairs(data.nodes) do
        local trafo = womf.Transform()
        if node.translation then
            trafo:setPosition(unpack(node.translation))
        end
        if node.rotation then
            trafo:setOrientation(unpack(node.rotation))
        end
        if node.scale then
            trafo:setScale(unpack(node.scale))
        end

        ret.nodes[nodeIdx] = {
            name = node.name,
            transform = trafo,
            mesh = node.mesh and ret.meshes[node.mesh + 1],
            skin = (node.mesh and node.skin) and ret.skins[node.skin + 1],
            children = {},
        }

        if node.mesh and node.skin then
            ret.skins[node.skin + 1].rootNode = ret.nodes[nodeIdx]
        end
    end

    for skinIdx, skin in ipairs(data.skins or {}) do
        local joints = {}
        for i, nodeIdx in ipairs(skin.joints) do
            joints[i] = {
                node = ret.nodes[nodeIdx + 1],
                inverseBindMatrix = mat4(),
            }
            assert(joints[i].node.name)
            joints[joints[i].node.name] = joints[i]
        end

        -- if not present inverse bind matrices are all identity matrices
        if skin.inverseBindMatrices then
            local accessor = data.accessors[skin.inverseBindMatrices + 1]
            assert(typeMap[accessor.componentType] == womf.attrType.f32)
            assert(accessor.type == "MAT4")
            assert(accessor.count == #skin.joints)

            local bufferView = ret.bufferViews[accessor.bufferView + 1]
            assert(bufferView:getSize() >= 16 * 4 * accessor.count)

            local ptr = ffi.cast("float*", bufferView:getPointer())
            for i = 1, #joints do
                joints[i].inverseBindMatrix = makeMat4(ptr + (i - 1) * 16)
            end
        end

        ret.skins[skinIdx].joints = joints
    end

    for nodeIdx, node in ipairs(data.nodes) do
        for i, childIdx in ipairs(node.children or {}) do
            local child = ret.nodes[childIdx + 1]
            ret.nodes[nodeIdx].children[i] = child
            child.parent = ret.nodes[nodeIdx]
        end
    end

    for i, nodeIdx in ipairs(data.scenes[1].nodes) do
        ret[i] = ret.nodes[nodeIdx + 1]
    end

    local interpMap = {
        STEP = womf.interp.step,
        LINEAR = womf.interp.linear,
    }

    local pathAccTypeMap = {
        translation = "VEC3",
        rotation = "VEC4",
        scale = "VEC3",
    }

    local pathSamplerTypeMap = {
        translation = womf.samplerType.vec3,
        rotation = womf.samplerType.quat,
        scale = womf.samplerType.vec3,
    }

    ret.animations = {}
    for animIdx, animation in ipairs(data.animations or {}) do
        local anim = womf.Animation()

        for _, channel in ipairs(animation.channels) do
            local key = data.nodes[channel.target.node + 1].name .. "/" .. channel.target.path
            local sampler = animation.samplers[channel.sampler + 1]
            local timesAcc = data.accessors[sampler.input + 1]
            assert(typeMap[timesAcc.componentType] == womf.attrType.f32)
            assert(timesAcc.type == "SCALAR")
            local timesBv = ret.bufferViews[timesAcc.bufferView + 1]
            local valuesAcc = data.accessors[sampler.output + 1]
            assert(typeMap[valuesAcc.componentType] == womf.attrType.f32)
            assert(valuesAcc.type == pathAccTypeMap[channel.target.path])
            local valuesBv = ret.bufferViews[valuesAcc.bufferView + 1]
            anim:addChannel(key, pathSamplerTypeMap[channel.target.path], interpMap[sampler.interpolation], timesBv, valuesBv)
        end

        ret.animations[animIdx] = anim
        if animation.name then
            ret.animations[animation.name] = anim
        end
    end

    ret.walk = walkScene
    ret.draw = drawScene

    return ret
end
