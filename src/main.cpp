#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <cmrc/cmrc.hpp>
CMRC_DECLARE(luaSource);

#include "animation.hpp"
#include "buffer.hpp"
#include "die.hpp"
#include "graphics.hpp"
#include "sdlw.hpp"
#include "util.hpp"

const std::unordered_map<std::string, sdlw::Keycode>& getKeycodeMap();
const std::unordered_map<sdlw::Keycode, std::string>& getInvKeycodeMap();

sdlw::Keycode getKeycode(const std::string& key)
{
    const auto it = getKeycodeMap().find(key);
    if (it == getKeycodeMap().end()) {
        die("Invalid key '{}'", key);
    }
    return it->second;
}

struct InputState {
    bool down = false;
    bool pressed = false;
    bool released = false;
};

std::array<InputState, SDL_NUM_SCANCODES> keyboardState;

InputState& getKeyState(sdlw::Scancode scancode)
{
    return keyboardState[static_cast<int>(scancode)];
}

InputState& getMouseState(const std::string& button)
{
    static std::array<InputState, 5> state;
    if (button == "l") {
        return state[0];
    } else if (button == "m") {
        return state[1];
    } else if (button == "r") {
        return state[2];
    } else if (button == "x1") {
        return state[3];
    } else if (button == "x2") {
        return state[4];
    }
    die("Invalid mouse button '{}'", button);
    return state[0]; // Just for the compiler
}

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

    table["isMouseDown"] = [](const std::string& button) { return getMouseState(button).down; };
    table["isMousePressed"]
        = [](const std::string& button) { return getMouseState(button).pressed; };
    table["isMouseReleased"]
        = [](const std::string& button) { return getMouseState(button).released; };

    table["getKeyFromScancode"] = [](int scancode) {
        return getInvKeycodeMap().at(sdlw::toKeycode(static_cast<sdlw::Scancode>(scancode)));
    };
    table["getScancodeFromKey"] = [](const std::string& key) {
        return static_cast<int>(sdlw::toScancode(getKeycode(key)));
    };

    table["isKeyDown"] = sol::overload(
        [](const std::string& key) { return getKeyState(toScancode(getKeycode(key))).down; },
        [](int scancode) { return getKeyState(static_cast<sdlw::Scancode>(scancode)).down; });
    table["isKeyPressed"] = sol::overload(
        [](const std::string& key) { return getKeyState(toScancode(getKeycode(key))).pressed; },
        [](int scancode) { return getKeyState(static_cast<sdlw::Scancode>(scancode)).pressed; });
    table["isKeyReleased"] = sol::overload(
        [](const std::string& key) { return getKeyState(toScancode(getKeycode(key))).released; },
        [](int scancode) { return getKeyState(static_cast<sdlw::Scancode>(scancode)).released; });

    table["pollEvent"] = [&lua]() {
        for (auto& state : keyboardState) {
            state.pressed = false;
            state.released = false;
        }

        return [&lua]() -> sol::object {
            using namespace sdlw::events;

            while (true) {
                const auto event = sdlw::pollEvent();
                if (!event) {
                    return sol::nil;
                }
                if (const auto quit = std::get_if<Quit>(&*event)) {
                    return lua.create_table_with("type", "quit");
                } else if (const auto resized = std::get_if<WindowResized>(&*event)) {
                    return lua.create_table_with("type", "windowresized", "width", resized->width,
                        "height", resized->height);
                } else if (const auto ctrlButtonDown = std::get_if<ControllerButtonDown>(&*event)) {
                    return lua.create_table_with("type", "controllerbuttondown", "joystick",
                        ctrlButtonDown->joystick, "button", sdlw::toString(ctrlButtonDown->button));
                } else if (const auto ctrlButtonUp = std::get_if<ControllerButtonUp>(&*event)) {
                    return lua.create_table_with("type", "controllerbuttonup", "joystick",
                        ctrlButtonUp->joystick, "button", sdlw::toString(ctrlButtonUp->button));
                } else if (const auto ctrlAxisMoved = std::get_if<ControllerAxisMoved>(&*event)) {
                    return lua.create_table_with("type", "controlleraxismoved", "joystick",
                        ctrlAxisMoved->joystick, "axis", sdlw::toString(ctrlAxisMoved->axis),
                        "value", ctrlAxisMoved->value);
                } else if (const auto joyAdded = std::get_if<JoystickAdded>(&*event)) {
                    return lua.create_table_with(
                        "type", "joystickadded", "joystick", joyAdded->joystick);
                } else if (const auto joyRemoved = std::get_if<JoystickRemoved>(&*event)) { // :(
                    return lua.create_table_with(
                        "type", "joystickremoved", "joystick", joyRemoved->joystick);
                } else if (const auto joyButtonDown = std::get_if<JoystickButtonDown>(&*event)) {
                    return lua.create_table_with("type", "joystickbuttondown", "joystick",
                        joyButtonDown->joystick, "button", joyButtonDown->button);
                } else if (const auto joyButtonUp = std::get_if<JoystickButtonUp>(&*event)) {
                    return lua.create_table_with("type", "joystickbuttonup", "joystick",
                        joyButtonUp->joystick, "button", joyButtonUp->button);
                } else if (const auto joyAxisMoved = std::get_if<JoystickAxisMoved>(&*event)) {
                    return lua.create_table_with("type", "joystickaxismoved", "joystick",
                        joyAxisMoved->joystick, "axis", joyAxisMoved->axis, "value",
                        joyAxisMoved->value);
                } else if (const auto keydown = std::get_if<KeyDown>(&*event)) {
                    if (!keydown->repeat) {
                        keyboardState[static_cast<int>(keydown->key.scancode)].pressed = true;
                        keyboardState[static_cast<int>(keydown->key.scancode)].down = true;

                        // There is some bug in lua.create_table_with, that will make it segfault if
                        // I pass all the values (the address of the last boolean will be 0x8).
                        auto event = lua.create_table_with("type", "keydown");
                        event["symbol"] = getInvKeycodeMap().at(keydown->key.symbol);
                        event["scancode"] = static_cast<int>(keydown->key.scancode);
                        event["isrepeat"] = keydown->repeat;
                        return event;
                    }
                } else if (const auto keyup = std::get_if<KeyUp>(&*event)) {
                    if (!keyup->repeat) {
                        keyboardState[static_cast<int>(keyup->key.scancode)].released = true;
                        keyboardState[static_cast<int>(keyup->key.scancode)].down = false;

                        auto event = lua.create_table_with("type", "keyup");
                        event["symbol"] = getInvKeycodeMap().at(keyup->key.symbol);
                        event["scancode"] = static_cast<int>(keyup->key.scancode);
                        event["isrepeat"] = keyup->repeat;
                        return event;
                    }
                }
            }
        };
    };

    auto joystick = lua.new_usertype<sdlw::Joystick>("Joystick");
    joystick["isConnected"] = &sdlw::Joystick::isConnected;
    joystick["getId"] = &sdlw::Joystick::getId;
    joystick["isController"] = &sdlw::Joystick::isController;
    joystick["getButton"]
        = [](const sdlw::Joystick& joystick, const std::string& controllerButton) {
              return joystick.getButton(sdlw::controllerButtonFromString(controllerButton));
          };
    joystick["getAxis"] = [](const sdlw::Joystick& joystick, const std::string& controllerAxis) {
        return joystick.getAxis(sdlw::controllerAxisFromString(controllerAxis));
    };

    // I thought about storing weak references to the joystick objects in the registry, so I can
    // return the same userdata object for the same joystick object for every call to getJoystick(s)
    // (and for every joystick event), but it seems sol2 overrides __eq for me so that userdata will
    // be equal if the shared_ptr they contain points to the same object.
    // This has some additional overhead from creating a new shared_ptr every time (esp. relevant
    // for events), but apart from that it *just works*, which is very rad and certainly good enough
    // for now!
    table["getJoysticks"] = []() { return sol::as_table(sdlw::getJoysticks()); };
    table["getJoystick"] = &sdlw::getJoystick;
}

void readUniform(
    UniformSet& uniformSet, const std::string& name, glw::UniformInfo::Type type, sol::object value)
{
    switch (type) {
    case glw::UniformInfo::Type::Float:
        dieAssert(value.get_type() == sol::type::number, "Value for '{}' must be 'number'", name);
        uniformSet[name] = value.as<float>();
        break;
    case glw::UniformInfo::Type::Int:
        dieAssert(value.get_type() == sol::type::number, "Value for '{}' must be 'number'", name);
        uniformSet[name] = value.as<int>();
        break;
    case glw::UniformInfo::Type::Vec4: {
        dieAssert(value.get_type() == sol::type::table, "Value for '{}' must be a 'table'", name);
        auto table = value.as<sol::table>();
        dieAssert(table.size() == 4, "Value for '{}' must have 4 elements", name);
        uniformSet[name] = glm::vec4(table[1], table[2], table[3], table[4]);
        break;
    }
    case glw::UniformInfo::Type::Mat4: {
        dieAssert(value.get_type() == sol::type::table, "Value for '{}' must be a 'table'", name);
        auto table = value.as<sol::table>();
        dieAssert(table.size() == 16, "Value for '{}' must have 16 elements", name);
        glm::mat4 mat;
        for (size_t i = 1; i <= 16; ++i) {
            mat[(i - 1) / 4][(i - 1) % 4] = table[i];
        }
        uniformSet[name] = mat;
        break;
    }
    case glw::UniformInfo::Type::Sampler2D:
        dieAssert(value.is<Texture>(), "Value for '{}' must be 'Texture'", name);
        uniformSet[name] = &value.as<Texture>().getGlTexture();
        break;
    default:
        die("Uniform of type '{}' not implemented yet", static_cast<GLenum>(type));
    }
}

UniformSet readUniforms(
    const std::unordered_map<std::string, glw::UniformInfo>& uniformInfo, sol::table uniforms)
{
    UniformSet uniformSet;
    for (auto&& [name, value] : uniforms) {
        const auto nameStr = name.as<std::string>();
        const auto infoIt = uniformInfo.find(nameStr);
        if (infoIt == uniformInfo.end()) {
            continue;
        }
        if (infoIt->second.size > 1) {
            dieAssert(value.get_type() == sol::type::table,
                "Value for '{}' must be 'table' (array size {})", nameStr, infoIt->second.size);
            auto table = value.as<sol::table>();
            for (size_t i = 1; i <= table.size(); ++i) {
                const auto elemName = fmt::format("{}[{}]", nameStr, i - 1);
                readUniform(uniformSet, elemName, infoIt->second.type, table[i]);
            }
        } else {
            readUniform(uniformSet, nameStr, infoIt->second.type, value);
        }
    }
    return uniformSet;
}

void bindGfx(sol::state&, sol::table table)
{
    table["clear"] = sol::overload(clearColor, clearColorDepth);
    table["flush"] = &flush;
    table["setProjectionMatrix"]
        = sol::overload(static_cast<void (*)(float, float, float, float)>(&setProjectionMatrix),
            static_cast<void (*)(float, float, float, float, float, float, float, float, float,
                float, float, float, float, float, float, float)>(&setProjectionMatrix));
    table["setViewMatrix"] = sol::overload(static_cast<void (*)(const Transform&)>(&setViewMatrix),
        static_cast<void (*)(float, float, float, float, float, float, float, float, float, float,
            float, float, float, float, float, float)>(&setViewMatrix));
    table["setModelMatrix"]
        = sol::overload(static_cast<void (*)(const Transform&)>(&setModelMatrix),
            static_cast<void (*)(float, float, float, float, float, float, float, float, float,
                float, float, float, float, float, float, float)>(&setModelMatrix));

    // TODO: optional RenderState, optional sortKey
    table["draw"] = [](Shader::Ptr shader, Geometry::Ptr geometry, sol::table uniforms) {
        draw(shader.get(), geometry.get(),
            readUniforms(shader->getProgram().getUniformInfo(), uniforms));
    };
}

auto bindBuffer(sol::state& lua)
{
    auto buffer = lua.new_usertype<Buffer>("Buffer", sol::base_classes, sol::bases<BufferBase>(),
        sol::call_constructor, sol::factories(&Buffer::create<std::string>));
    buffer["getSize"] = &Buffer::size;
    return buffer;
}

auto bindBufferView(sol::state& lua)
{
    auto bufferView = lua.new_usertype<BufferView>("BufferView", sol::base_classes,
        sol::bases<BufferBase>(), sol::call_constructor,
        sol::factories(
            static_cast<BufferView::Ptr (*)(Buffer::Ptr, size_t, size_t)>(&BufferView::create),
            static_cast<BufferView::Ptr (*)(BufferView::Ptr, size_t, size_t)>(
                &BufferView::create)));
    bufferView["getSize"] = &BufferView::size;
    return bufferView;
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
    trafo["getMatrix"] = &Transform::getMatrix;
    return trafo;
}

auto bindSampler(sol::state& lua)
{
    auto sampler = lua.new_usertype<Sampler>("Sampler", sol::call_constructor,
        sol::constructors<Sampler(Sampler::Type, Interpolation, Buffer::Ptr, Buffer::Ptr),
            Sampler(Sampler::Type, Interpolation, BufferView::Ptr, BufferView::Ptr)>());
    sampler["getType"] = &Sampler::getType;
    sampler["getDuration"] = &Sampler::getDuration;
    sampler["getInterpolation"] = &Sampler::getInterpolation;
    sampler["sample"] = [](sol::this_state L, const Sampler& sampler, float time) {
        const auto v = sampler.sample(time);
        // This is a vector. I'd rather make the manual push work but I don't know how.
        sol::variadic_results res;
        if (const auto scalar = std::get_if<float>(&v)) {
            res.emplace_back(L, sol::in_place, *scalar);
            // return sol::stack::push(L, *scalar);
        } else if (const auto vec3 = std::get_if<glm::vec3>(&v)) {
            res.emplace_back(L, sol::in_place, vec3->x);
            res.emplace_back(L, sol::in_place, vec3->y);
            res.emplace_back(L, sol::in_place, vec3->z);
            // return sol::stack::multi_push(L, vec3->x, vec3->y, vec3->z);
        } else if (const auto quat = std::get_if<glm::quat>(&v)) {
            res.emplace_back(L, sol::in_place, quat->x);
            res.emplace_back(L, sol::in_place, quat->y);
            res.emplace_back(L, sol::in_place, quat->z);
            res.emplace_back(L, sol::in_place, quat->w);
            // return sol::stack::multi_push(L, quat->x, quat->y, quat->z, quat->w);
        } else {
            // return 0;
        }
        return res;
    };
    return sampler;
}

extern "C" {
const void* Buffer_getPointer(const void* obj)
{
    const auto buf = reinterpret_cast<const Buffer::Ptr*>(obj);
    return (*buf)->data().data();
}

const void* BufferView_getPointer(const void* obj)
{
    const auto buf = reinterpret_cast<const BufferView::Ptr*>(obj);
    return (*buf)->data().data();
}
}

void bindTypes(sol::state& lua, sol::table table)
{
    // For some reason I can't get shared_ptr<Buffer(View)> to convert to shared_ptr<BufferBase>
    // :(
    lua.new_usertype<BufferBase>("BufferBase");
    table["Buffer"] = bindBuffer(lua);
    table["BufferView"] = bindBufferView(lua);

    lua.script(R"(
        ffi.cdef [[
        const void* Buffer_getPointer(const void* obj);
        const void* BufferView_getPointer(const void* obj);
        ]]

        function womf.Buffer:getPointer()
            return ffi.C.Buffer_getPointer(self)
        end

        function womf.BufferView:getPointer()
            return ffi.C.BufferView_getPointer(self)
        end
    )");

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

    lua.new_enum("InterpolationType", "step", Interpolation::Step, "linear", Interpolation::Linear);
    table["interp"] = lua["InterpolationType"];
    lua["InterpolationType"] = sol::nil;

    lua.new_enum("SamplerType", "scalar", Sampler::Type::Scalar, "vec3", Sampler::Type::Vec3,
        "quat", Sampler::Type::Quat);
    table["samplerType"] = lua["SamplerType"];
    lua["SamplerType"] = sol::nil;

    table["Sampler"] = bindSampler(lua);
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

std::string_view getCmrcFile(const std::string& filename)
{
    static const auto resFs = cmrc::luaSource::get_filesystem();
    const auto file = resFs.open(filename);
    return std::string_view(file.begin(), file.end());
}

sol::load_result load(sol::state_view lua, std::string_view code, const std::string& moduleName)
{
    auto res = lua.load(code, moduleName);
    if (!res.valid()) {
        die("{}", res.get<sol::error>().what());
    }
    return res;
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

    if (args.size() > 0) {
        lua["args"] = std::vector<std::string>(args.begin() + 1, args.end());
    } else {
        lua["args"] = lua.create_table();
    }

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

    sdlw::Sdl sdl(sdlw::SubSystem::Everything);
    if (sdl < 0) {
        fmt::print(stderr, "Could not initialize SDL: {}\n", sdlw::getError());
        return 1;
    }
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
    lua.add_package_loader(
        [&resFs](sol::this_state L, const std::string& moduleName) -> sol::object {
            std::string path = moduleName;
            std::replace(path.begin(), path.end(), '.', '/');
            if (resFs.is_file(path + ".lua")) {
                return load(L, getCmrcFile(path + ".lua"), moduleName);
            } else if (resFs.is_directory(path) && resFs.is_file(path + "/" + "init.lua")) {
                return load(L, getCmrcFile(path + "/" + "init.lua"), moduleName);
            }
            return sol::make_object(L, "Module not found");
        });

    lua["womf"] = lua.create_table();
    bindSys(lua, lua["womf"], *window);
    bindGfx(lua, lua["womf"]);
    bindTypes(lua, lua["womf"]);
    lua["womf"]["readFile"] = &readFile<std::string>;

    auto init = lua.script(getCmrcFile("init.lua"), "init");
    if (!init.valid()) {
        fmt::print(stderr, "Error: {}\n", init.get<sol::error>().what());
        return 1;
    }

    auto main = lua.script_file("main.lua");
    if (!main.valid()) {
        fmt::print(stderr, "Error: {}\n", main.get<sol::error>().what());
        return 1;
    }
    const auto mainRes = main.get<sol::function>()();
    if (!mainRes.valid()) {
        fmt::print(stderr, "Error running main.lua: {}\n", mainRes.get<sol::error>().what());
        return 1;
    }

    return 0;
}
