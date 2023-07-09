#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <cmrc/cmrc.hpp>
CMRC_DECLARE(luaSource);

#include "buffer.hpp"
#include "die.hpp"
#include "graphics.hpp"
#include "sdlw.hpp"
#include "util.hpp"

void bindSys(sol::state& lua, sol::table table, const sdlw::GlWindow& window)
{
    table["getTime"] = &sdlw::getTime;
    table["present"] = [&window]() {
        flush();
        window.swap();
    };
    table["getWindowSize"] = [&window]() {
        const auto [w, h] = window.getSize();
        return std::tuple { w, h };
    };
    table["isDown"] = [](int key) { return sdlw::isDown(static_cast<sdlw::Keycode>(key)); };
    table["pollEvent"] = [&lua]() {
        return [&lua]() -> sol::object {
            const auto event = sdlw::pollEvent();
            if (!event) {
                return sol::nil;
            }
            if (const auto quit = std::get_if<sdlw::events::Quit>(&*event)) {
                return lua.create_table_with("type", "quit");
            } else if (const auto keydown = std::get_if<sdlw::events::KeyDown>(&*event)) {
                return lua.create_table_with("type", "keydown", "symbol",
                    static_cast<int>(keydown->key.symbol), "scancode",
                    static_cast<int>(keydown->key.scancode));
            } else if (const auto resized = std::get_if<sdlw::events::WindowResized>(&*event)) {
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
        const auto& uniformInfo = shader->getProgram().getUniformInfo();
        UniformSet uniformSet;
        for (auto&& [name, value] : uniforms) {
            const auto nameStr = name.as<std::string>();
            const auto infoIt = uniformInfo.find(nameStr);
            if (infoIt == uniformInfo.end()) {
                continue;
            }
            switch (infoIt->second.type) {
            case glw::UniformInfo::Type::Float:
                dieAssert(value.get_type() == sol::type::number, "Value for '{}' must be 'number'",
                    nameStr);
                uniformSet[nameStr] = value.as<float>();
                break;
            case glw::UniformInfo::Type::Int:
                dieAssert(value.get_type() == sol::type::number, "Value for '{}' must be 'number'",
                    nameStr);
                uniformSet[nameStr] = value.as<int>();
                break;
            case glw::UniformInfo::Type::Vec4: {
                dieAssert(value.get_type() == sol::type::table, "Value for '{}' must be a 'table'",
                    nameStr);
                auto tbl = value.as<sol::table>();
                dieAssert(tbl.size() == 4, "Value for '{}' must have 4 elements", nameStr);
                uniformSet[nameStr] = glm::vec4(tbl[1], tbl[2], tbl[3], tbl[4]);
                break;
            }
            case glw::UniformInfo::Type::Sampler2D:
                dieAssert(value.is<Texture>(), "Value for '{}' must be 'Texture'", nameStr);
                uniformSet[nameStr] = &value.as<Texture>().getGlTexture();
                break;
            default:
                die("Uniform of type '{}' not implemented yet",
                    static_cast<GLenum>(infoIt->second.type));
            }
        }
        draw(shader.get(), geometry.get(), trafo, uniformSet);
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
            }
            return fmt;
        }));
}

auto bindGeometry(sol::state& lua)
{
    auto geometry = lua.new_usertype<Geometry>(
        "Geometry", sol::call_constructor, sol::factories(&Geometry::create));
    geometry["addVertexBuffer"] = &Geometry::addVertexBuffer;
    geometry["setIndexBuffer"] = &Geometry::setIndexBuffer;
    return geometry;
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
    table["Shader"] = bindShader(lua);

    table["Texture"] = bindTexture(lua);
    table["pixelTexture"] = [](float r, float g, float b, float a) {
        return Texture::createPixel(glm::vec4(r, g, b, a));
    };

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

void preload(sol::state& lua, const std::string& moduleName, const cmrc::embedded_filesystem& resFs,
    const std::string& filename)
{
    lua["package"]["preload"][moduleName] = [&, moduleName, filename]() -> sol::object {
        try {
            auto file = resFs.open(filename);
            return lua.script(std::string_view(file.begin(), file.end()));
        } catch (const std::exception& exc) {
            die("Could not preload '{}': {}", moduleName, exc.what());
            return sol::nil;
        }
    };
}

int main(int argc, char** argv)
{
    std::vector<std::string_view> args(argv + 1, argv + argc);
    if (args.size() > 0) {
        std::filesystem::current_path(args[0]);
    }

    sol::state lua;
    lua.set_exception_handler(solExceptionHandler);
    // This is everything, except io, os and debug
    lua.open_libraries(sol::lib::base, sol::lib::bit32, sol::lib::coroutine, sol::lib::coroutine,
        sol::lib::ffi, sol::lib::jit, sol::lib::math, sol::lib::package, sol::lib::string,
        sol::lib::table);

    std::optional<std::string> title;
    std::optional<uint32_t> width;
    std::optional<uint32_t> height;
    try {
        auto config = lua.script_file("config.lua");
        assert(config.valid());
        auto table = config.get<sol::table>();
        title = table["title"].get<std::optional<std::string>>();
        width = table["width"].get<std::optional<uint32_t>>();
        height = table["height"].get<std::optional<uint32_t>>();
    } catch (const sol::error& exc) {
        fmt::print(stderr, "Error loading config.lua: {}\n", exc.what());
        return 1;
    }

    // SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

    const auto window = sdlw::GlWindow::create(
        title.value_or("womf"), width.value_or(1024), height.value_or(768), { .resizable = true });
    if (!window) {
        fmt::print(stderr, "Error creating window: {}\n", sdlw::getError());
        return 1;
    }
    // window->setSwapInterval(1);

    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
    glw::State::instance().setViewport(window->getSize().x, window->getSize().y);
    glEnable(GL_DEPTH_TEST);

    const auto resFs = cmrc::luaSource::get_filesystem();
    preload(lua, "json", resFs, "json.lua");
    preload(lua, "gltf", resFs, "gltf.lua");

    // Just require this into the global table. I need it *all the time*.
    auto inspectLua = resFs.open("inspect.lua");
    lua["inspect"] = lua.script(std::string_view(inspectLua.begin(), inspectLua.end()));

    lua["womf"] = lua.create_table();
    bindSys(lua, lua["womf"], *window);
    bindGfx(lua, lua["womf"]);
    bindTypes(lua, lua["womf"]);
    lua["womf"]["readFile"] = &readFile<std::string>;

    try {
        auto main = lua.script_file("main.lua");
        if (!main.valid()) {
            fmt::print(stderr, "Error: {}\n", main.get<sol::error>().what());
            return 1;
        }
        const auto res = main.get<sol::function>()();
        if (!res.valid()) {
            fmt::print(stderr, "Error running main.lua: {}\n", res.get<sol::error>().what());
            return 1;
        }
    } catch (const DieException& exc) {
        fmt::print(stderr, "{}\n", exc.what());
        return 1;
    }

    return 0;
}
