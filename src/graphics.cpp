#include "graphics.hpp"

#include <algorithm>

#include "die.hpp"

// Windows is so fucking stupid
#undef near
#undef far

Texture::Ptr Texture::create(Buffer::Ptr buffer)
{
    return std::shared_ptr<Texture>(
        new Texture(std::static_pointer_cast<BufferBase>(std::move(buffer))));
}

Texture::Ptr Texture::create(BufferView::Ptr buffer)
{
    return std::shared_ptr<Texture>(
        new Texture(std::static_pointer_cast<BufferBase>(std::move(buffer))));
}

Texture::Ptr Texture::createPixel(const glm::vec4& color, size_t width, size_t height)
{
    return std::shared_ptr<Texture>(new Texture(glwx::makeTexture2D(color, width, height)));
}

const glw::Texture& Texture::getGlTexture() const
{
    return texture_;
}

Texture::Texture(BufferBase::Ptr buffer)
    : buffer_(std::move(buffer))
{
    auto texture = glwx::makeTexture2D(buffer_->data().data(), buffer_->data().size());
    if (!texture) {
        throw DieException(fmt::format("Could not load texture '{}'", buffer_->name()));
    }
    texture_ = std::move(*texture);
}

Texture::Texture(std::string path)
    : Texture(Buffer::create(std::move(path)))
{
}

Texture::Texture(glw::Texture texture)
    : texture_(std::move(texture))
{
}

namespace {
struct Line {
    std::string_view line;
    size_t afterNewline;
};

Line getLine(std::string_view str, size_t cursor)
{
    const auto nl = std::min(str.size(), str.find_first_of("\n\r", cursor));
    const auto line = str.substr(cursor, nl - cursor);
    auto afterNewline = nl;
    if (afterNewline < str.size() && str[afterNewline] == '\n')
        afterNewline++;
    if (afterNewline < str.size() && str[afterNewline] == '\r')
        afterNewline++;
    return Line { line, afterNewline };
}

std::string_view trim(std::string_view str)
{
    const auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) {
        return {};
    }
    const auto end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string getDirectory(std::string_view path)
{
    const auto lastSep = path.rfind('/');
    if (lastSep == std::string_view::npos) {
        return "./";
    }
    return std::string(path.substr(0, lastSep + 1));
}

std::optional<std::string> resolveIncludes(std::string_view src, const std::string& filePath)
{
    std::string output;
    output.reserve(src.size());
    size_t cursor = 0;
    size_t start = 0;
    size_t lineNumber = 1;
    while (cursor < src.size()) {
        cursor = src.find_first_not_of(" \t", cursor);
        if (cursor >= src.size())
            break;
        const auto [line, afterNewline] = getLine(src, cursor);
        if (line.size() > 0 && line[0] == '#') {
            const auto trimmed = trim(line.substr(1));
            const auto directiveEnd = trimmed.find_first_not_of("include");
            const auto directive = trimmed.substr(0, directiveEnd);
            if (directive == "include") {
                const auto arg = trim(trimmed.substr(directiveEnd));
                std::string path;
                if (arg.size() > 1 && arg[0] == '"' && arg[arg.size() - 1] == '"') {
                    // relative include
                    path = getDirectory(filePath);
                    path.append(arg.substr(1, arg.size() - 2));
                } else if (arg.size() > 1 && arg[0] == '<' && arg[arg.size() - 1] == '>') {
                    // absolute include
                    path = arg.substr(1, arg.size() - 2);
                } else {
                    fmt::print(
                        stderr, "Invalid argument '{}' for #include in line {}", arg, lineNumber);
                    return std::nullopt;
                }
                const auto included = glwx::readFile(path);
                if (!included) {
                    fmt::print(stderr, "Could not load included shader: {}", path);
                    return std::nullopt;
                }
                output.append(src.substr(start, cursor - start));
                output.append("#line 1\n");
                output.append(*included);
                output.append("\n#line ");
                output.append(std::to_string(lineNumber + 1));
                output.append("\n");
                start = afterNewline;
            }
        }
        cursor = afterNewline;
        lineNumber++;
    }
    output.append(src.substr(start));
    return output;
}
}

Shader::Ptr Shader::create(Buffer::Ptr vert, Buffer::Ptr frag)
{
    return std::shared_ptr<Shader>(new Shader(std::static_pointer_cast<BufferBase>(std::move(vert)),
        std::static_pointer_cast<BufferBase>(std::move(frag))));
}

Shader::Ptr Shader::create(Buffer::Ptr combined)
{
    return std::shared_ptr<Shader>(
        new Shader(std::static_pointer_cast<BufferBase>(std::move(combined))));
}

const glw::ShaderProgram& Shader::getProgram() const
{
    return prog_;
}

void Shader::initialize(std::string_view vert, std::string_view vertPath, std::string_view frag,
    std::string_view fragPath)
{
    auto vertFull = resolveIncludes(vert, std::string(vertPath));
    if (!vertFull) {
        throw DieException(fmt::format("Could not resolve includes for shader: {}", vertPath));
    }

    auto fragFull = resolveIncludes(frag, std::string(fragPath));
    if (!fragFull) {
        throw DieException(fmt::format("Could not resolve includes for shader: {}", fragPath));
    }

    auto prog = glwx::makeShaderProgram(vert, frag);
    if (!prog) {
        throw DieException(
            fmt::format("Could not create shader '{}' (vert) / '{}' (frag)", vertPath, fragPath));
    }
    prog_ = std::move(*prog);
}

Shader::Shader(BufferBase::Ptr vert, BufferBase::Ptr frag)
{
    const std::string_view vertSv(
        reinterpret_cast<const char*>(vert->data().data()), vert->data().size());

    const std::string_view fragSv(
        reinterpret_cast<const char*>(frag->data().data()), frag->data().size());

    initialize(vertSv, vert->path(), fragSv, frag->path());
}

Shader::Shader(BufferBase::Ptr combined)
{
    const std::string_view sv(
        reinterpret_cast<const char*>(combined->data().data()), combined->data().size());
    std::string vert = "#define VERTEX_SHADER\n" + std::string(sv);
    std::string frag = "#define FRAGMENT_SHADER\n#define FROGMENT_SHADER\n" + std::string(sv);

    initialize(vert, combined->path(), frag, combined->path());
}

Shader::Shader(std::string vertPath, std::string fragPath)
    : Shader(Buffer::create(vertPath), Buffer::create(fragPath))
{
}

Shader::Shader(std::string combined)
    : Shader(Buffer::create(combined))
{
}

GraphicsBuffer::Ptr GraphicsBuffer::create(
    BufferTarget target, BufferUsage usage, Buffer::Ptr buffer)
{
    return std::shared_ptr<GraphicsBuffer>(
        new GraphicsBuffer(target, usage, std::static_pointer_cast<BufferBase>(std::move(buffer))));
}

GraphicsBuffer::Ptr GraphicsBuffer::create(
    BufferTarget target, BufferUsage usage, BufferView::Ptr buffer)
{
    return std::shared_ptr<GraphicsBuffer>(
        new GraphicsBuffer(target, usage, std::static_pointer_cast<BufferBase>(std::move(buffer))));
}

GraphicsBuffer::Ptr GraphicsBuffer::create(
    BufferTarget target, BufferUsage usage, std::string filename)
{
    return std::shared_ptr<GraphicsBuffer>(new GraphicsBuffer(target, usage, std::move(filename)));
}

glw::Buffer& GraphicsBuffer::getGlBuffer()
{
    return gfxBuffer_;
}

GraphicsBuffer::GraphicsBuffer(BufferTarget target, BufferUsage usage, BufferBase::Ptr buffer)
    : target_(target)
    , usage_(usage)
    , buffer_(std::move(buffer))
{
    gfxBuffer_.data(static_cast<glw::Buffer::Target>(target_),
        static_cast<glw::Buffer::UsageHint>(usage_), buffer_->data().data(),
        buffer_->data().size());
}

GraphicsBuffer::GraphicsBuffer(BufferTarget target, BufferUsage usage, std::string filename)
    : GraphicsBuffer(target, usage, Buffer::create(std::move(filename)))
{
}

Geometry::Ptr Geometry::create(glw::DrawMode mode)
{
    return std::shared_ptr<Geometry>(new Geometry(mode));
}

void Geometry::draw()
{
    primitive_.draw();
}

Geometry::Geometry(glw::DrawMode mode)
    : primitive_(mode)
{
}

void Geometry::addVertexBuffer(const glw::VertexFormat& fmt, GraphicsBuffer::Ptr buffer)
{
    primitive_.addVertexBuffer(buffer->getGlBuffer(), fmt);
    vertexBuffers_.push_back(std::move(buffer));
}

void Geometry::setIndexBuffer(glw::AttributeType idxType, GraphicsBuffer::Ptr buffer)
{
    primitive_.setIndexBuffer(buffer->getGlBuffer(), static_cast<glw::IndexType>(idxType));
    indexBuffer_ = std::move(buffer);
}

std::tuple<float, float, float> Transform::unpack(const glm::vec3& v)
{
    return { v.x, v.y, v.z };
}

std::tuple<float, float, float, float> Transform::unpack(const glm::quat& q)
{
    return { q.x, q.y, q.z, q.w };
}

std::tuple<float, float, float> Transform::getPosition() const
{
    return unpack(glwx::Transform::getPosition());
}

void Transform::setPosition(float x, float y, float z)
{
    glwx::Transform::setPosition(glm::vec3(x, y, z));
}

void Transform::move(float x, float y, float z)
{
    glwx::Transform::move(glm::vec3(x, y, z));
}

void Transform::moveLocal(float x, float y, float z)
{
    glwx::Transform::moveLocal(glm::vec3(x, y, z));
}

std::tuple<float, float, float> Transform::getScale() const
{
    return unpack(glwx::Transform::getScale());
}

void Transform::setScale(float x, float y, float z)
{
    glwx::Transform::setScale(glm::vec3(x, y, z));
}

std::tuple<float, float, float, float> Transform::getOrientation() const
{
    return unpack(glwx::Transform::getOrientation());
}

void Transform::setOrientation(float w, float x, float y, float z)
{
    glwx::Transform::setOrientation(glm::quat(w, x, y, z));
}

void Transform::rotate(float w, float x, float y, float z)
{
    glwx::Transform::rotate(glm::quat(w, x, y, z));
}

void Transform::rotateLocal(float w, float x, float y, float z)
{
    glwx::Transform::rotateLocal(glm::quat(w, x, y, z));
}

std::tuple<float, float, float> Transform::localToWorld(float x, float y, float z) const
{
    return unpack(glwx::Transform::localToWorld(glm::vec3(x, y, z)));
}

std::tuple<float, float, float> Transform::getForward() const
{
    return unpack(glwx::Transform::getForward());
}

std::tuple<float, float, float> Transform::getRight() const
{
    return unpack(glwx::Transform::getRight());
}

std::tuple<float, float, float> Transform::getUp() const
{
    return unpack(glwx::Transform::getUp());
}

void Transform::lookAt(float x, float y, float z)
{
    glwx::Transform::lookAt(glm::vec3(x, y, z));
}

void Transform::lookAt(float x, float y, float z, float upX, float upY, float upZ)
{
    glwx::Transform::lookAt(glm::vec3(x, y, z), glm::vec3(upX, upY, upZ));
}

glm::mat4 Transform::getMatrix() const
{
    return glwx::Transform::getMatrix();
}

size_t getAttributeLocation(const std::string& name)
{
    static const std::unordered_map<std::string, size_t> locs {
        { "position", 0 },
        { "normal", 1 },
        { "tangent", 2 },
        { "texcoord0", 3 },
        { "texcoord1", 4 },
        { "color0", 5 },
        { "joints0", 6 },
        { "weights0", 7 },
    };
    const auto it = locs.find(name);
    assert(it != locs.end());
    return it->second;
}

void clearColor(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void clearColorDepth(float r, float g, float b, float a, float depth)
{
    glClearColor(r, g, b, a);
    glClearDepth(depth);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

namespace {
glm::mat4 modelMatrix;
glm::mat4 invModelMatrix;
glm::mat3 normalMatrix; // inv normal is just transpose
glm::mat4 viewMatrix;
glm::mat4 invViewMatrix;
glm::mat4 projectionMatrix;
glm::mat4 invProjectionMatrix;

glm::mat4 modelViewMatrix;
glm::mat4 invModelViewMatrix;
glm::mat4 viewProjectionMatrix;
glm::mat4 invViewProjectionMatrix;
glm::mat4 modelViewProjectionMatrix;
glm::mat4 invModelViewProjectionMatrix;

void updateMV()
{
    modelViewMatrix = viewMatrix * modelMatrix;
    invModelViewMatrix = invModelMatrix * invViewMatrix;
}

void updateVP()
{
    viewProjectionMatrix = projectionMatrix * viewMatrix;
    invViewProjectionMatrix = invViewMatrix * invProjectionMatrix;
}

void updateMVP()
{
    modelViewProjectionMatrix = viewProjectionMatrix * modelMatrix;
    invModelViewProjectionMatrix = invModelMatrix * invViewProjectionMatrix;
}
}

void setModelMatrix(const glm::mat4& mat)
{
    modelMatrix = mat;
    invModelMatrix = glm::inverse(mat);
    normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
    updateMV();
    updateMVP();
}

void setModelMatrix(const Transform& trafo)
{
    setModelMatrix(trafo.getMatrix());
}

void setViewMatrix(const glm::mat4& mat)
{
    viewMatrix = mat;
    invViewMatrix = glm::inverse(mat); // useless double inversion
    updateMV();
    updateVP();
    updateMVP();
}

void setViewMatrix(const Transform& trafo)
{
    setViewMatrix(glm::inverse(trafo.getMatrix()));
}

void setProjectionMatrix(const glm::mat4& mat)
{
    projectionMatrix = mat;
    invProjectionMatrix = glm::inverse(projectionMatrix);
    updateVP();
    updateMVP();
}

void setProjectionMatrix(float fovy, float aspect, float near, float far)
{
    setProjectionMatrix(glm::perspective(fovy, aspect, near, far));
}

int bind(const glw::Texture* texture)
{
    static GLint numTextureUnits = 0;
    if (numTextureUnits == 0) {
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &numTextureUnits);
        dieAssert(numTextureUnits > 0, "Maximum number of texture units is 0");
    }
    struct BoundTexture {
        int unit = -1;
        const glw::Texture* texture = nullptr;
    };
    // This is sorted by most-recently-used first
    static auto boundTextures = [&]() {
        std::vector<BoundTexture> bound(numTextureUnits);
        for (int i = 0; i < numTextureUnits; ++i) {
            bound[i].unit = i;
        }
        return bound;
    }();
    auto moveToFront = [&](size_t index) {
        const auto it = boundTextures.begin() + index;
        std::rotate(boundTextures.begin(), it, it + 1);
    };
    for (size_t i = 0; i < boundTextures.size(); ++i) {
        if (boundTextures[i].texture == texture) {
            moveToFront(i);
            return boundTextures[0].unit;
        } else if (!boundTextures[i].texture) {
            moveToFront(i);
            texture->bind(boundTextures[0].unit);
            boundTextures[0].texture = texture;
            return boundTextures[0].unit;
        }
    }
    // Texture isn't already bound and there is no free texture unit
    moveToFront(boundTextures.size() - 1); // Move last (least recently used) to front
    texture->bind(boundTextures[0].unit);
    boundTextures[0].texture = texture;
    return boundTextures[0].unit;
}

void UniformSet::set(const glw::ShaderProgram& shader) const
{
    for (const auto& [name, value] : uniforms) {
        std::visit(
            [&](auto&& v) {
                if constexpr (std::is_same_v<std::decay_t<decltype(v)>, const glw::Texture*>) {
                    shader.setUniform(name, bind(v));
                } else {
                    shader.setUniform(name, v);
                }
            },
            value);
    }
}

void draw(Shader* shader, Geometry* geometry, const UniformSet& uniforms)
{
    const auto& prog = shader->getProgram();
    prog.bind();

    prog.setUniform("modelMatrix", modelMatrix);
    prog.setUniform("invModelMatrix", invModelMatrix);
    prog.setUniform("normalMatrix", normalMatrix);
    prog.setUniform("viewMatrix", viewMatrix);
    prog.setUniform("invViewMatrix", invViewMatrix);
    prog.setUniform("projectionMatrix", projectionMatrix);
    prog.setUniform("invProjectionMatrix", invProjectionMatrix);

    prog.setUniform("modelViewMatrix", modelViewMatrix);
    prog.setUniform("invModelViewMatrix", invModelViewMatrix);
    prog.setUniform("viewProjectionMatrix", viewProjectionMatrix);
    prog.setUniform("invViewProjectionMatrix", invViewProjectionMatrix);
    prog.setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
    prog.setUniform("invModelViewProjectionMatrix", invModelViewProjectionMatrix);

    uniforms.set(prog);

    geometry->draw();
}

void flush() { }
