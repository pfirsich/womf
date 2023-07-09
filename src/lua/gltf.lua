local json = require "json"

local gltf = {}

local shader = womf.Shader("assets/default.vert", "assets/default.frag")
local ident = womf.Transform()

local function drawNode(node)
    if node.mesh then
        for _, prim in ipairs(node.mesh.primitives) do
            womf.draw(shader, prim.geometry, ident, {
                texture = prim.material.albedo,
            })
        end
    end
    for _, child in ipairs(node.children) do
        drawNode(node) -- TODO: Transform!
    end
end

local function drawScene(scene)
    for _, node in ipairs(scene) do
        drawNode(node)
    end
end

function gltf.load(filename)
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

    ret.nodes = {}
    for nodeIdx, node in ipairs(data.nodes) do
        local trafo = womf.Transform()
        if node.translation then
            trafo:setPosition(unpack(node.translation))
        end
        if node.rotation then
            local x, y, z, w = unpack(node.rotation)
            trafo:setOrientation(w, x, y, z)
        end
        if node.scale then
            trafo:setScale(unpack(node.scale))
        end

        ret.nodes[nodeIdx] = {
            name = node.name,
            transform = trafo,
            mesh = node.mesh and ret.meshes[node.mesh + 1],
            children = {},
        }
    end

    for nodeIdx, node in ipairs(data.nodes) do
        for i, childIdx in ipairs(node.children or {}) do
            local child = ret.nodes[childIdx + 1]
            ret.nodes[nodeIdx].children[i] = child
            child.parent = ret.nodes[nodeIdx]
        end
    end

    for _, nodeIdx in ipairs(data.scenes[1].nodes) do
        table.insert(ret, ret.nodes[nodeIdx + 1])
    end

    ret.draw = drawScene

    return ret
end

return gltf
