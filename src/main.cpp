#include <cstdio>
#include <exception>
#include <filesystem>
#include <span>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#define SOL_ALL_SAFETIES_ON 1
#include <sol.hpp>

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

#include "sdlw.hpp"

glm::mat4 projectionMatrix;
glm::mat4 invProjectionMatrix;
glm::mat4 viewMatrix;
glm::mat4 invViewMatrix;
glm::mat4 viewProjectionMatrix;
glm::mat4 invViewProjectionMatrix;

class DieException : public std::runtime_error {
public:
    template <typename Msg>
    explicit DieException(Msg&& msg)
        : std::runtime_error(std::forward<Msg>(msg))
    {
        // I need 3.3.0 for call constructors, but there is a bug in 3.3.0 that prevents C++
        // exceptions from bubbling up so the error message inside them is lost:
        // https://github.com/ThePhD/sol2/issues/1508
        // This is why I do this weird shit here.
        fmt::print("Error: {}\n", msg);
        std::exit(1);
    }
};

void clearColor([[maybe_unused]] float r, [[maybe_unused]] float g, [[maybe_unused]] float b,
    [[maybe_unused]] float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void clearColorDepth([[maybe_unused]] float r, [[maybe_unused]] float g, [[maybe_unused]] float b,
    [[maybe_unused]] float a, [[maybe_unused]] float depth)
{
    glClearColor(r, g, b, a);
    glClearDepth(depth);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void setProjection([[maybe_unused]] float fovy, [[maybe_unused]] float aspect,
    [[maybe_unused]] float near, [[maybe_unused]] float far)
{
    projectionMatrix = glm::perspective(fovy, aspect, near, far);
    invProjectionMatrix = glm::inverse(projectionMatrix);
    viewProjectionMatrix = projectionMatrix * viewMatrix;
    invViewProjectionMatrix = invViewMatrix * invProjectionMatrix;
}

void flush() { }

class BufferBase {
public:
    using Ptr = std::shared_ptr<BufferBase>;

    virtual ~BufferBase() = default;
    virtual std::span<const uint8_t> data() const = 0;
    virtual size_t size() const = 0;
    virtual std::string path() const = 0;
    virtual std::string name() const = 0;
};

class Buffer final
    : public BufferBase
    , public std::enable_shared_from_this<Buffer> {
public:
    using Ptr = std::shared_ptr<Buffer>;

    template <typename... Args>
    [[nodiscard]] static Ptr create(Args&&... args)
    {
        return std::shared_ptr<Buffer>(new Buffer(std::forward<Args>(args)...));
    }

    std::span<const uint8_t> data() const override { return std::span<const uint8_t>(data_); }

    size_t size() const override { return 0; }

    std::string path() const override { return filename_; }
    std::string name() const override { return filename_; }

private:
    Buffer(std::string filename)
        : filename_(std::move(filename))
    {
        auto file = std::unique_ptr<FILE, decltype(&std::fclose)>(
            std::fopen(filename_.c_str(), "rb"), &std::fclose);
        if (!file) {
            throw DieException(fmt::format("Could not open file '{}'", filename_));
        }
        std::fseek(file.get(), 0, SEEK_END);
        data_.resize(std::ftell(file.get()));
        std::fseek(file.get(), 0, SEEK_SET);
        std::fread(data_.data(), 1, data_.size(), file.get());
    }

    std::vector<uint8_t> data_;
    std::string filename_;
};

class BufferView final
    : public BufferBase
    , public std::enable_shared_from_this<BufferView> {
public:
    using Ptr = std::shared_ptr<BufferView>;

    [[nodiscard]] static Ptr create(Buffer::Ptr buffer, size_t offset, size_t size)
    {
        return std::shared_ptr<BufferView>(
            new BufferView(std::static_pointer_cast<BufferBase>(std::move(buffer)), offset, size));
    }

    [[nodiscard]] static Ptr create(BufferView::Ptr buffer, size_t offset, size_t size)
    {
        return std::shared_ptr<BufferView>(
            new BufferView(std::static_pointer_cast<BufferBase>(std::move(buffer)), offset, size));
    }

    std::span<const uint8_t> data() const override
    {
        return buffer_->data().subspan(offset_, size_);
    }

    size_t size() const override { return size_; }

    std::string path() const override { return buffer_->path(); }

    std::string name() const override
    {
        return buffer_->name() + fmt::format("[{}:{}]", offset_, size_);
    }

private:
    BufferView(BufferBase::Ptr buffer, size_t offset = 0, size_t size = -1)
        : buffer_(std::move(buffer))
        , offset_(offset)
        , size_(std::min(size, buffer_->size()))
    {
    }

    BufferBase::Ptr buffer_;
    size_t offset_;
    size_t size_;
};

class Texture : public std::enable_shared_from_this<Texture> {
public:
    using Ptr = std::shared_ptr<Texture>;

    constexpr static std::string_view type() { return "Texture"; }

    // I can't use templates, because sol2 will get confused and I can't just take a BufferBase,
    // because sol2 will validate the types and will error out.

    [[nodiscard]] static Ptr create(Buffer::Ptr buffer)
    {
        return std::shared_ptr<Texture>(
            new Texture(std::static_pointer_cast<BufferBase>(std::move(buffer))));
    }

    [[nodiscard]] static Ptr create(BufferView::Ptr buffer)
    {
        return std::shared_ptr<Texture>(
            new Texture(std::static_pointer_cast<BufferBase>(std::move(buffer))));
    }

    template <typename... Args>
    [[nodiscard]] static Ptr create(Args&&... args)
    {
        return std::shared_ptr<Texture>(new Texture(std::forward<Args>(args)...));
    }

    const glw::Texture& getGlTexture() const { return texture_; }

private:
    Texture(BufferBase::Ptr buffer)
        : buffer_(std::move(buffer))
    {
        auto texture = glwx::makeTexture2D(buffer_->data().data(), buffer_->data().size());
        if (!texture) {
            throw DieException(fmt::format("Could not load texture '{}'", buffer_->name()));
        }
        texture_ = std::move(*texture);
    }

    Texture(std::string path)
        : Texture(Buffer::create(std::move(path)))
    {
    }

    BufferBase::Ptr buffer_;
    glw::Texture texture_;
};

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
                    spdlog::critical(
                        "Invalid argument '{}' for #include in line {}", arg, lineNumber);
                    return std::nullopt;
                }
                const auto included = glwx::readFile(path);
                if (!included) {
                    spdlog::critical("Could not load included shader: {}", path);
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

class Shader : public std::enable_shared_from_this<Shader> {
public:
    using Ptr = std::shared_ptr<Shader>;

    [[nodiscard]] static Ptr create(Buffer::Ptr vert, Buffer::Ptr frag)
    {
        return std::shared_ptr<Shader>(
            new Shader(std::static_pointer_cast<BufferBase>(std::move(vert)),
                std::static_pointer_cast<BufferBase>(std::move(frag))));
    }

    [[nodiscard]] static Ptr create(Buffer::Ptr combined)
    {
        return std::shared_ptr<Shader>(
            new Shader(std::static_pointer_cast<BufferBase>(std::move(combined))));
    }

    template <typename... Args>
    [[nodiscard]] static Ptr create(Args&&... args)
    {
        return std::shared_ptr<Shader>(new Shader(std::forward<Args>(args)...));
    }

    const glw::ShaderProgram& getProgram() const { return prog_; }

private:
    void initialize(std::string_view vert, std::string_view vertPath, std::string_view frag,
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
            throw DieException(fmt::format(
                "Could not create shader '{}' (vert) / '{}' (frag)", vertPath, fragPath));
        }
        prog_ = std::move(*prog);
    }

    Shader([[maybe_unused]] BufferBase::Ptr vert, [[maybe_unused]] BufferBase::Ptr frag)
    {
        const std::string_view vertSv(
            reinterpret_cast<const char*>(vert->data().data()), vert->data().size());

        const std::string_view fragSv(
            reinterpret_cast<const char*>(frag->data().data()), frag->data().size());

        initialize(vertSv, vert->path(), fragSv, frag->path());
    }

    Shader([[maybe_unused]] BufferBase::Ptr combined)
    {
        const std::string_view sv(
            reinterpret_cast<const char*>(combined->data().data()), combined->data().size());
        std::string vert = "#define VERTEX_SHADER\n" + std::string(sv);
        std::string frag = "#define FRAGMENT_SHADER\n#define FROGMENT_SHADER\n" + std::string(sv);

        initialize(vert, combined->path(), frag, combined->path());
    }

    Shader(std::string vertPath, std::string fragPath)
        : Shader(Buffer::create(vertPath), Buffer::create(fragPath))
    {
    }

    Shader([[maybe_unused]] std::string combined)
        : Shader(Buffer::create(combined))
    {
    }

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

    [[nodiscard]] static Ptr create(BufferTarget target, BufferUsage usage, Buffer::Ptr buffer)
    {
        return std::shared_ptr<GraphicsBuffer>(new GraphicsBuffer(
            target, usage, std::static_pointer_cast<BufferBase>(std::move(buffer))));
    }

    [[nodiscard]] static Ptr create(BufferTarget target, BufferUsage usage, BufferView::Ptr buffer)
    {
        return std::shared_ptr<GraphicsBuffer>(new GraphicsBuffer(
            target, usage, std::static_pointer_cast<BufferBase>(std::move(buffer))));
    }

    [[nodiscard]] static Ptr create(BufferTarget target, BufferUsage usage, std::string filename)
    {
        return std::shared_ptr<GraphicsBuffer>(
            new GraphicsBuffer(target, usage, std::move(filename)));
    }

    glw::Buffer& getGlBuffer() { return gfxBuffer_; }

private:
    GraphicsBuffer(BufferTarget target, BufferUsage usage, BufferBase::Ptr buffer)
        : target_(target)
        , usage_(usage)
        , buffer_(std::move(buffer))
    {
    }

    GraphicsBuffer(BufferTarget target, BufferUsage usage, std::string filename)
        : target_(target)
        , usage_(usage)
        , buffer_(Buffer::create(std::move(filename)))
    {
        gfxBuffer_.data(static_cast<glw::Buffer::Target>(target_),
            static_cast<glw::Buffer::UsageHint>(usage_), buffer_->data().data(),
            buffer_->data().size());
    }

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
        GraphicsBuffer::Ptr attributes, glw::AttributeType idxType, GraphicsBuffer::Ptr indices)
    {
        return std::shared_ptr<Geometry>(new Geometry(mode, std::move(fmt), std::move(attributes),
            static_cast<glw::IndexType>(idxType), std::move(indices)));
    }

    void draw() { primitive_.draw(); }

private:
    Geometry(glw::DrawMode mode, glw::VertexFormat fmt, GraphicsBuffer::Ptr attributes,
        glw::IndexType idxType, GraphicsBuffer::Ptr indices)
        : mode_(mode)
        , fmt_(std::move(fmt))
        , attributes_(std::move(attributes))
        , idxType_(idxType)
        , indices_(std::move(indices))
        , primitive_(mode)
    {
        primitive_.addVertexBuffer(attributes_->getGlBuffer(), fmt_);
        primitive_.setIndexBuffer(indices_->getGlBuffer(), idxType_);
    }

    glw::DrawMode mode_;
    glw::VertexFormat fmt_;
    GraphicsBuffer::Ptr attributes_;
    glw::IndexType idxType_;
    GraphicsBuffer::Ptr indices_;
    glwx::Primitive primitive_;
};

struct Transform : public glwx::Transform {
    Transform() = default;

    static std::tuple<float, float, float> unpack(const glm::vec3& v) { return { v.x, v.y, v.z }; }

    static std::tuple<float, float, float, float> unpack(const glm::quat& q)
    {
        return { q.x, q.y, q.z, q.w };
    }

    std::tuple<float, float, float> getPosition() const
    {
        return unpack(glwx::Transform::getPosition());
    }

    void setPosition(float x, float y, float z)
    {
        glwx::Transform::setPosition(glm::vec3(x, y, z));
    }

    void move(float x, float y, float z) { glwx::Transform::move(glm::vec3(x, y, z)); }

    void moveLocal(float x, float y, float z) { glwx::Transform::moveLocal(glm::vec3(x, y, z)); }

    std::tuple<float, float, float> getScale() const { return unpack(glwx::Transform::getScale()); }

    void setScale(float x, float y, float z) { glwx::Transform::setScale(glm::vec3(x, y, z)); }

    std::tuple<float, float, float, float> getOrientation() const
    {
        return unpack(glwx::Transform::getOrientation());
    }

    void setOrientation(float w, float x, float y, float z)
    {
        glwx::Transform::setOrientation(glm::quat(w, x, y, z));
    }

    void rotate(float w, float x, float y, float z)
    {
        glwx::Transform::rotate(glm::quat(w, x, y, z));
    }

    void rotateLocal(float w, float x, float y, float z)
    {
        glwx::Transform::rotateLocal(glm::quat(w, x, y, z));
    }

    std::tuple<float, float, float> localToWorld(float x, float y, float z) const
    {
        return unpack(glwx::Transform::localToWorld(glm::vec3(x, y, z)));
    }

    std::tuple<float, float, float> getForward() const
    {
        return unpack(glwx::Transform::getForward());
    }

    std::tuple<float, float, float> getRight() const { return unpack(glwx::Transform::getRight()); }

    std::tuple<float, float, float> getUp() const { return unpack(glwx::Transform::getUp()); }

    void lookAt(float x, float y, float z) { glwx::Transform::lookAt(glm::vec3(x, y, z)); }

    void lookAt(float x, float y, float z, float upX, float upY, float upZ)
    {
        glwx::Transform::lookAt(glm::vec3(x, y, z), glm::vec3(upX, upY, upZ));
    }
};

void setView(const Transform& trafo)
{
    const auto mat = trafo.getMatrix();
    viewMatrix = glm::inverse(mat);
    invViewMatrix = mat;
    viewProjectionMatrix = projectionMatrix * viewMatrix;
    invViewProjectionMatrix = invViewMatrix * invProjectionMatrix;
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

void bindSys(sol::state& lua, sol::table table, const sdlw::GlWindow& window)
{
    table["getTime"] = &sdlw::getTime;
    table["present"] = [&window]() { window.swap(); };
    table["getWindowSize"] = [&window]() {
        const auto [w, h] = window.getSize();
        return std::tuple { w, h };
    };
    table["pollEvent"] = [&lua]() {
        return [&lua]() -> sol::object {
            fmt::print("cpp womf.pollEvent\n");
            const auto event = sdlw::pollEvent();
            if (!event) {
                return sol::nil;
            }
            if (const auto quit = std::get_if<sdlw::events::Quit>(&*event)) {
                fmt::print("cpp quit\n");
                return lua.create_table_with("type", "quit");
            } else if (const auto keydown = std::get_if<sdlw::events::KeyDown>(&*event)) {
                fmt::print("cpp keydown\n");
                return lua.create_table_with("type", "keydown", "symbol",
                    static_cast<int>(keydown->key.symbol), "scancode",
                    static_cast<int>(keydown->key.scancode));
            } else if (const auto resized = std::get_if<sdlw::events::WindowResized>(&*event)) {
                fmt::print("cpp resized\n");
                return lua.create_table_with(
                    "type", "windowresized", "width", resized->width, "height", resized->height);
            }
            return sol::nil;
        };
    };
}

void bindGfx(sol::state&, sol::table table)
{
    table["clear"] = sol::overload(clearColor, clearColorDepth);
    table["flush"] = &flush;
    table["setProjection"] = &setProjection;
    table["setView"] = &setView;

    // TODO: optional RenderState, optional sortKey
    table["draw"] = [](Shader::Ptr shader, Geometry::Ptr geometry, const Transform& trafo,
                        sol::table uniforms) {
        const auto modelMatrix = trafo.getMatrix();
        const auto normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
        const auto modelViewMatrix = viewMatrix * modelMatrix;
        const auto modelViewProjectionMatrix = viewProjectionMatrix * modelMatrix;
        const auto& prog = shader->getProgram();
        prog.bind();
        prog.setUniform("modelMatrix", modelMatrix);
        prog.setUniform("normalMatrix", normalMatrix);
        prog.setUniform("viewMatrix", viewMatrix);
        prog.setUniform("projectionMatrix", projectionMatrix);
        prog.setUniform("modelViewMatrix", modelViewMatrix);
        prog.setUniform("viewProjectionMatrix", viewProjectionMatrix);
        prog.setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
        int unit = 0;
        for (auto&& [name, value] : uniforms) {
            if (value.get_type() == sol::type::number) {
                // float, int
                // const float v = value;
                // const int v = value;
            } else if (value.get_type() == sol::type::table) {
                // vec2, vec3, vec4, mat2, mat3, mat4
            } else if (value.is<Texture>()) {
                value.as<Texture>().getGlTexture().bind(unit);
                prog.setUniform(name.as<std::string>(), unit++);
            }
        }
        geometry->draw();
    };
}

auto bindBuffer(sol::state& lua)
{
    return lua.new_usertype<Buffer>("Buffer", sol::base_classes, sol::bases<BufferBase>(),
        sol::call_constructor, sol::factories(&Buffer::create<std::string>));
}

auto bindBufferView(sol::state& lua)
{
    return lua.new_usertype<BufferView>("BufferView", sol::base_classes, sol::bases<BufferBase>(),
        sol::call_constructor,
        sol::factories(
            static_cast<BufferView::Ptr (*)(Buffer::Ptr, size_t, size_t)>(&BufferView::create),
            static_cast<BufferView::Ptr (*)(BufferView::Ptr, size_t, size_t)>(
                &BufferView::create)));
}

auto bindTexture(sol::state& lua)
{
    return lua.new_usertype<Texture>("Texture", sol::call_constructor,
        sol::factories(static_cast<Texture::Ptr (*)(Buffer::Ptr)>(&Texture::create),
            static_cast<Texture::Ptr (*)(BufferView::Ptr)>(&Texture::create),
            &Texture::create<std::string>));
}

auto bindShader(sol::state& lua)
{
    return lua.new_usertype<Shader>("Shader", sol::call_constructor,
        sol::factories(static_cast<Shader::Ptr (*)(Buffer::Ptr, Buffer::Ptr)>(&Shader::create),
            static_cast<Shader::Ptr (*)(Buffer::Ptr)>(&Shader::create),
            &Shader::create<std::string, std::string>, &Shader::create<std::string>));
}

auto bindGraphicsBuffer(sol::state& lua)
{
    return lua.new_usertype<GraphicsBuffer>("GraphicsBuffer", sol::call_constructor,
        sol::factories(static_cast<GraphicsBuffer::Ptr (*)(BufferTarget, BufferUsage, Buffer::Ptr)>(
                           &GraphicsBuffer::create),
            static_cast<GraphicsBuffer::Ptr (*)(BufferTarget, BufferUsage, BufferView::Ptr)>(
                &GraphicsBuffer::create),
            static_cast<GraphicsBuffer::Ptr (*)(BufferTarget, BufferUsage, std::string)>(
                &GraphicsBuffer::create)));
}

auto bindVertexFormat(sol::state& lua)
{
    return lua.new_usertype<glw::VertexFormat>(
        "VertexFormat", sol::call_constructor, sol::factories([](sol::table table) {
            glw::VertexFormat fmt;
            for (auto& elem : table) {
                const auto attr = elem.second.as<sol::table>();
                const auto loc = [&]() {
                    if (attr.get<sol::object>(1).get_type() == sol::type::string) {
                        return getAttributeLocation(attr.get<std::string>(1));
                    } else if (attr.get<sol::object>(1).get_type() == sol::type::number) {
                        return attr.get<size_t>(1);
                    } else {
                        assert(false);
                    }
                }();
                const auto type = attr.get<glw::AttributeType>(2);
                const auto num = attr.get<uint32_t>(3);
                const auto normalized = attr.get_or(4, false);
                fmt.add(loc, num, type, normalized);
                fmt::print("loc: {}, type: {}, num: {}, normalized: {}\n", loc,
                    static_cast<int>(type), num, normalized);
            }
            return fmt;
        }));
}

auto bindGeometry(sol::state& lua)
{
    return lua.new_usertype<Geometry>("Geometry", sol::call_constructor,
        sol::factories(static_cast<Geometry::Ptr (*)(glw::DrawMode, glw::VertexFormat,
                GraphicsBuffer::Ptr, glw::AttributeType, GraphicsBuffer::Ptr)>(&Geometry::create)));
}

auto bindTransform(sol::state& lua)
{
    auto trafo = lua.new_usertype<Transform>(
        "Transform", sol::call_constructor, sol::constructors<Transform()>());
    trafo["getPosition"] = &Transform::getPosition;
    trafo["setPosition"] = &Transform::setPosition;
    trafo["move"] = &Transform::move;
    trafo["moveLocal"] = &Transform::moveLocal;
    trafo["getScale"] = &Transform::getScale;
    trafo["setScale"] = &Transform::setScale;
    trafo["getOrientation"] = &Transform::getOrientation;
    trafo["setOrientation"] = &Transform::setOrientation;
    trafo["rotate"] = &Transform::rotate;
    trafo["rotateLocal"] = &Transform::rotateLocal;
    trafo["localToWorld"] = &Transform::localToWorld;
    trafo["getForward"] = &Transform::getForward;
    trafo["getRight"] = &Transform::getRight;
    trafo["getUp"] = &Transform::getUp;
    trafo["lookAt"]
        = sol::overload(static_cast<void (Transform::*)(float, float, float)>(&Transform::lookAt),
            static_cast<void (Transform::*)(float, float, float, float, float, float)>(
                &Transform::lookAt));
    return trafo;
}

void bindTypes(sol::state& lua, sol::table table)
{
    // For some reason I can't get shared_ptr<Buffer(View)> to convert to shared_ptr<BufferBase>
    // :(
    lua.new_usertype<BufferBase>("BufferBase");
    table["Buffer"] = bindBuffer(lua);
    table["BufferView"] = bindBufferView(lua);
    table["Texture"] = bindTexture(lua);
    table["Shader"] = bindShader(lua);

    lua.new_enum(
        "BufferTarget", "attributes", BufferTarget::Attributes, "indices", BufferTarget::Indices);
    table["bufferTarget"] = lua["BufferTarget"];
    lua["BufferTarget"] = sol::nil;

    lua.new_enum("BufferUsage", "static", BufferUsage::Static, "dynamic", BufferUsage::Dynamic,
        "stream", BufferUsage::Stream);
    table["bufferUsage"] = lua["BufferUsage"];
    lua["BufferUsage"] = sol::nil;

    table["GraphicsBuffer"] = bindGraphicsBuffer(lua);

    lua.new_enum("AttributeType", "i8", glw::AttributeType::I8, "u8", glw::AttributeType::U8, "i16",
        glw::AttributeType::I16, "u16", glw::AttributeType::U16, "i32", glw::AttributeType::I32,
        "u32", glw::AttributeType::U32, "f16", glw::AttributeType::F16, "f32",
        glw::AttributeType::F32, "f64", glw::AttributeType::F64);
    table["attrType"] = lua["AttributeType"];
    lua["AttributeType"] = sol::nil;

    table["VertexFormat"] = bindVertexFormat(lua);

    lua.new_enum("DrawMode", "points", glw::DrawMode::Points, "lines", glw::DrawMode::Lines,
        "lineLoop", glw::DrawMode::LineLoop, "lineStrip", glw::DrawMode::LineStrip, "triangles",
        glw::DrawMode::Triangles, "triangleFan", glw::DrawMode::TriangleFan, "triangleStrip",
        glw::DrawMode::TriangleStrip);
    table["drawMode"] = lua["DrawMode"];
    lua["DrawMode"] = sol::nil;

    table["Geometry"] = bindGeometry(lua);
    table["Transform"] = bindTransform(lua);
}

int solExceptionHandler(
    lua_State* L, sol::optional<const std::exception&> exc, sol::string_view description)
{
    if (exc) {
        fmt::print(stderr, "Error: {}\n", exc->what());
    } else {
        fmt::print(stderr, "Error: {}\n", description);
    }
    std::exit(1);

    return sol::stack::push(L, description);
}

int main(int argc, char** argv)
{
    std::vector<std::string_view> args(argv + 1, argv + argc);
    if (args.size() > 0) {
        std::filesystem::current_path(args[0]);
    }

    sol::state lua;
    lua.set_exception_handler(solExceptionHandler);
    lua.open_libraries(sol::lib::base, sol::lib::ffi);

    std::optional<std::string> title;
    try {
        auto config = lua.script_file("config.lua");
        assert(config.valid());
        title = config.get<sol::table>()["title"];
    } catch (const sol::error& exc) {
        fmt::print(stderr, "Error loading config.lua: {}\n", exc.what());
    }

    // SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

    const auto window
        = sdlw::GlWindow::create(title.value_or("womf"), 1024, 768, { .resizable = true });
    if (!window) {
        fmt::print(stderr, "Error creating window: {}\n", sdlw::getError());
        return 1;
    }
    // window->setSwapInterval(1);

    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
    glw::State::instance().setViewport(window->getSize().x, window->getSize().y);

    lua["womf"] = lua.create_table();
    bindSys(lua, lua["womf"], *window);
    bindGfx(lua, lua["womf"]);
    bindTypes(lua, lua["womf"]);

    try {
        auto main = lua.script_file("main.lua");
        if (!main.valid()) {
            fmt::print(stderr, "Error: {}\n", static_cast<sol::error>(main).what());
            return 1;
        }
        const auto res = main.get<sol::function>()();
        if (!res.valid()) {
            fmt::print(stderr, "Error running main.lua: {}\n", static_cast<sol::error>(res).what());
            return 1;
        }
    } catch (const DieException& exc) {
        fmt::print(stderr, "{}\n", exc.what());
        return 1;
    }

    return 0;
}
