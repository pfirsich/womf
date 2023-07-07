#pragma once

#include <variant>

#include "glw/enums.hpp"
#include "glw/state.hpp"
#include "glw/texture.hpp"
#include "glw/vertexformat.hpp"
#include "glwx/buffers.hpp"
#include "glwx/primitive.hpp"
#include "glwx/shader.hpp"
#include "glwx/texture.hpp"
#include "glwx/transform.hpp"
#include "glwx/utility.hpp"

#include "buffer.hpp"

class Texture : public std::enable_shared_from_this<Texture> {
public:
    using Ptr = std::shared_ptr<Texture>;

    // I can't use templates, because sol2 will get confused and I can't just take a BufferBase,
    // because sol2 will validate the types and will error out.

    [[nodiscard]] static Ptr create(Buffer::Ptr buffer);
    [[nodiscard]] static Ptr create(BufferView::Ptr buffer);

    template <typename... Args>
    [[nodiscard]] static Ptr create(Args&&... args)
    {
        return std::shared_ptr<Texture>(new Texture(std::forward<Args>(args)...));
    }

    const glw::Texture& getGlTexture() const;

private:
    Texture(BufferBase::Ptr buffer);
    Texture(std::string path);

    BufferBase::Ptr buffer_;
    glw::Texture texture_;
};

class Shader : public std::enable_shared_from_this<Shader> {
public:
    using Ptr = std::shared_ptr<Shader>;

    [[nodiscard]] static Ptr create(Buffer::Ptr vert, Buffer::Ptr frag);
    [[nodiscard]] static Ptr create(Buffer::Ptr combined);

    template <typename... Args>
    [[nodiscard]] static Ptr create(Args&&... args)
    {
        return std::shared_ptr<Shader>(new Shader(std::forward<Args>(args)...));
    }

    const glw::ShaderProgram& getProgram() const;

private:
    void initialize(std::string_view vert, std::string_view vertPath, std::string_view frag,
        std::string_view fragPath);

    Shader(BufferBase::Ptr vert, BufferBase::Ptr frag);
    Shader(BufferBase::Ptr combined);

    Shader(std::string vertPath, std::string fragPath);
    Shader(std::string combined);

    glw::ShaderProgram prog_;
};

template <typename Enum>
constexpr auto toUnderlying(Enum e)
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

enum class BufferTarget : std::underlying_type_t<glw::Buffer::Target> {
    Attributes = toUnderlying(glw::Buffer::Target::Array),
    Indices = toUnderlying(glw::Buffer::Target::ElementArray),
};

enum class BufferUsage : std::underlying_type_t<glw::Buffer::UsageHint> {
    Static = toUnderlying(glw::Buffer::UsageHint::StaticDraw),
    Dynamic = toUnderlying(glw::Buffer::UsageHint::DynamicDraw),
    Stream = toUnderlying(glw::Buffer::UsageHint::StreamDraw),
};

class GraphicsBuffer : public std::enable_shared_from_this<GraphicsBuffer> {
public:
    using Ptr = std::shared_ptr<GraphicsBuffer>;

    [[nodiscard]] static Ptr create(BufferTarget target, BufferUsage usage, Buffer::Ptr buffer);
    [[nodiscard]] static Ptr create(BufferTarget target, BufferUsage usage, BufferView::Ptr buffer);
    [[nodiscard]] static Ptr create(BufferTarget target, BufferUsage usage, std::string filename);

    glw::Buffer& getGlBuffer();

private:
    GraphicsBuffer(BufferTarget target, BufferUsage usage, BufferBase::Ptr buffer);
    GraphicsBuffer(BufferTarget target, BufferUsage usage, std::string filename);

    BufferTarget target_;
    BufferUsage usage_;
    BufferBase::Ptr buffer_;
    glw::Buffer gfxBuffer_;
};

class Geometry : public std::enable_shared_from_this<Geometry> {
public:
    using Ptr = std::shared_ptr<Geometry>;

    // This takes an AttributeType, so I only have to bind a single enum to Lua
    [[nodiscard]] static Ptr create(glw::DrawMode mode, glw::VertexFormat fmt,
        GraphicsBuffer::Ptr attributes, glw::AttributeType idxType, GraphicsBuffer::Ptr indices);

    void draw();

private:
    Geometry(glw::DrawMode mode, glw::VertexFormat fmt, GraphicsBuffer::Ptr attributes,
        glw::IndexType idxType, GraphicsBuffer::Ptr indices);

    glw::DrawMode mode_;
    glw::VertexFormat fmt_;
    GraphicsBuffer::Ptr attributes_;
    glw::IndexType idxType_;
    GraphicsBuffer::Ptr indices_;
    glwx::Primitive primitive_;
};

struct Transform : public glwx::Transform {
    Transform() = default;

    static std::tuple<float, float, float> unpack(const glm::vec3& v);
    static std::tuple<float, float, float, float> unpack(const glm::quat& q);

    std::tuple<float, float, float> getPosition() const;
    void setPosition(float x, float y, float z);

    void move(float x, float y, float z);
    void moveLocal(float x, float y, float z);

    std::tuple<float, float, float> getScale() const;
    void setScale(float x, float y, float z);

    std::tuple<float, float, float, float> getOrientation() const;
    void setOrientation(float w, float x, float y, float z);

    void rotate(float w, float x, float y, float z);
    void rotateLocal(float w, float x, float y, float z);

    std::tuple<float, float, float> localToWorld(float x, float y, float z) const;
    std::tuple<float, float, float> getForward() const;
    std::tuple<float, float, float> getRight() const;
    std::tuple<float, float, float> getUp() const;

    void lookAt(float x, float y, float z);
    void lookAt(float x, float y, float z, float upX, float upY, float upZ);
};

using UniformValue = std::variant<int, float, glm::vec2, glm::vec3, glm::vec4, glm::mat2, glm::mat3,
    glm::mat4, const glw::Texture*>;

// TODO: Use Uniform Buffer Objects
class UniformSet {
public:
    UniformValue& operator[](const std::string& name) { return uniforms[name]; }

    void set(const glw::ShaderProgram& shader) const;

private:
    std::unordered_map<std::string, UniformValue> uniforms;
};

size_t getAttributeLocation(const std::string& name);

void clearColor(float r, float g, float b, float a);
void clearColorDepth(float r, float g, float b, float a, float depth);

void setProjection(float fovy, float aspect, float near, float far);
void setView(const Transform& trafo);

void draw(Shader* shader, Geometry* geometry, const Transform& trafo, const UniformSet& uniforms);
void flush();
