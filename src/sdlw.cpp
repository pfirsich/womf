#include "sdlw.hpp"

#include <cassert>
#include <utility>

#include <SDL_syswm.h>

namespace sdlw {
Keycode toKeycode(Scancode scancode)
{
    return static_cast<Keycode>(SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(scancode)));
}

Scancode toScancode(Keycode key)
{
    return static_cast<Scancode>(SDL_GetScancodeFromKey(static_cast<SDL_Keycode>(key)));
}

bool isDown(Scancode scancode)
{
    static int numKeys = 0;
    static const auto state = SDL_GetKeyboardState(&numKeys);
    const auto idx = static_cast<SDL_Scancode>(scancode);
    return idx < numKeys && state[idx] > 0;
}

bool isDown(Keycode key)
{
    return isDown(toScancode(key));
}

MouseButtonState getMouseButtonState(uint32_t bitmask)
{
    return MouseButtonState {
        .left = (bitmask & SDL_BUTTON_LMASK) > 0,
        .middle = (bitmask & SDL_BUTTON_RMASK) > 0,
        .right = (bitmask & SDL_BUTTON_RMASK) > 0,
        .x1 = (bitmask & SDL_BUTTON_X1MASK) > 0,
        .x2 = (bitmask & SDL_BUTTON_X2MASK) > 0,
    };
}

MouseButtonState getMouseButtonState()
{
    return getMouseButtonState(SDL_GetMouseState(nullptr, nullptr));
}

MousePosition getMousePosition()
{
    int x = 0, y = 0;
    SDL_GetMouseState(&x, &y);
    return { .x = x, .y = y };
}

bool setRelativeMouseMode(bool enabled)
{
    return SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE) == 0;
}

bool getRelativeMouseMode()
{
    return SDL_GetRelativeMouseMode() == SDL_TRUE;
}

bool setCursorVisible(bool visible)
{
    return SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

bool isCursorVisible()
{
    return SDL_ShowCursor(SDL_QUERY) == SDL_ENABLE;
}

std::string_view toString(ControllerButton button)
{
    return std::string_view(
        SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(button)));
}

ControllerButton controllerButtonFromString(const std::string& str)
{
    return static_cast<ControllerButton>(SDL_GameControllerGetButtonFromString(str.c_str()));
}

std::string_view toString(ControllerAxis axis)
{
    return std::string_view(
        SDL_GameControllerGetStringForAxis(static_cast<SDL_GameControllerAxis>(axis)));
}

ControllerAxis controllerAxisFromString(const std::string& str)
{
    return static_cast<ControllerAxis>(SDL_GameControllerGetAxisFromString(str.c_str()));
}

int addControllerMappings(const std::string& mappings)
{
    const auto rw = SDL_RWFromConstMem(mappings.data(), mappings.size());
    return SDL_GameControllerAddMappingsFromRW(rw, 1);
}

Joystick::Ptr Joystick::internalOpen(int deviceIndex)
{
    auto joystick = Joystick::Ptr(new Joystick());
    if (!joystick->open(deviceIndex)) {
        return nullptr;
    }
    return joystick;
}

SDL_Joystick* Joystick::getSdlJoystick()
{
    return joystick_;
}

SDL_GameController* Joystick::getSdlController()
{
    return controller_;
}

bool Joystick::isOpen() const
{
    return joystick_ != nullptr;
}

void Joystick::close()
{
    if (controller_) {
        SDL_GameControllerClose(controller_);
    } else if (joystick_) {
        SDL_JoystickClose(joystick_);
    }
    controller_ = nullptr;
    joystick_ = nullptr;
}

bool Joystick::isConnected() const
{
    return isOpen() && SDL_JoystickGetAttached(joystick_);
}

SDL_JoystickID Joystick::getId() const
{
    return id_;
}

SDL_JoystickGUID Joystick::getGuid() const
{
    return SDL_JoystickGetGUID(joystick_);
}

std::string Joystick::getGuidString() const
{
    char buf[33];
    SDL_JoystickGetGUIDString(getGuid(), buf, sizeof(buf));
    return std::string(buf);
}

bool Joystick::isController() const
{
    return controller_ != nullptr;
}

namespace {
    float axisToFloat(int16_t v)
    {
        return v < 0 ? static_cast<float>(v) / std::numeric_limits<decltype(v)>::min()
                     : static_cast<float>(v) / std::numeric_limits<decltype(v)>::max();
    }
}

float Joystick::getAxis(ControllerAxis axis) const
{
    assert(controller_);
    return axisToFloat(
        SDL_GameControllerGetAxis(controller_, static_cast<SDL_GameControllerAxis>(axis)));
}

bool Joystick::getButton(ControllerButton button) const
{
    assert(controller_);
    return SDL_GameControllerGetButton(controller_, static_cast<SDL_GameControllerButton>(button));
}

bool Joystick::open(int index)
{
    if (SDL_IsGameController(index)) {
        controller_ = SDL_GameControllerOpen(index);
        if (!controller_) {
            return false;
        }
        joystick_ = SDL_GameControllerGetJoystick(controller_);
        assert(joystick_); // It seems the function above cannot fail?
    } else {
        joystick_ = SDL_JoystickOpen(index);
        if (!joystick_) {
            return false;
        }
    }
    id_ = SDL_JoystickInstanceID(joystick_);
    return true;
}

namespace {
    std::vector<Joystick::Ptr>& getConnectedJoysticks()
    {
        static std::vector<Joystick::Ptr> joysticks;
        return joysticks;
    }

    Joystick::Ptr addJoystick(int index)
    {
        if (getJoystick(SDL_JoystickGetDeviceInstanceID(index))) {
            return nullptr;
        }
        auto joystick = Joystick::internalOpen(index);
        if (!joystick) {
            return nullptr;
        }
        getConnectedJoysticks().push_back(joystick);
        return joystick;
    }

    Joystick::Ptr removeJoystick(SDL_JoystickID id)
    {
        auto& joysticks = getConnectedJoysticks();
        const auto it = std::find_if(joysticks.begin(), joysticks.end(),
            [id](const auto& joystick) { return id == joystick->getId(); });
        if (it == joysticks.end()) {
            return nullptr;
        }
        auto joystick = std::move(*it);
        joysticks.erase(it);
        return joystick;
    }
}

std::vector<Joystick::Ptr> getJoysticks()
{
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        addJoystick(i);
    }
    return getConnectedJoysticks();
}

Joystick::Ptr getJoystick(SDL_JoystickID id)
{
    auto& joysticks = getConnectedJoysticks();
    const auto it = std::find_if(joysticks.begin(), joysticks.end(),
        [id](const auto& joystick) { return id == joystick->getId(); });
    if (it == joysticks.end()) {
        return nullptr;
    }
    return *it;
}

std::optional<Event> pollEvent()
{
    SDL_Event event;
    if (!SDL_PollEvent(&event)) {
        return std::nullopt;
    }
    switch (event.type) {
    case SDL_QUIT:
        return detail::callEventCallbacks(events::Quit {});
    case SDL_APP_TERMINATING:
        break;
    case SDL_APP_LOWMEMORY:
        break;
    case SDL_APP_WILLENTERBACKGROUND:
        break;
    case SDL_APP_DIDENTERBACKGROUND:
        break;
    case SDL_APP_WILLENTERFOREGROUND:
        break;
    case SDL_APP_DIDENTERFOREGROUND:
        break;
    case SDL_LOCALECHANGED:
        break;
    case SDL_DISPLAYEVENT:
        break;
    case SDL_WINDOWEVENT:
        switch (event.window.type) {
        case SDL_WINDOWEVENT_SHOWN:
            return std::nullopt;
        case SDL_WINDOWEVENT_HIDDEN:
            return std::nullopt;
        case SDL_WINDOWEVENT_EXPOSED:
            return std::nullopt;
        case SDL_WINDOWEVENT_MOVED:
            return detail::callEventCallbacks(events::WindowMoved {
                .windowId = event.window.windowID,
                .x = event.window.data1,
                .y = event.window.data2,
            });
        case SDL_WINDOWEVENT_RESIZED:
            return detail::callEventCallbacks(events::WindowResized {
                .windowId = event.window.windowID,
                .width = event.window.data1,
                .height = event.window.data2,
            });
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            return detail::callEventCallbacks(events::WindowSizeChanged {
                .windowId = event.window.windowID,
                .width = event.window.data1,
                .height = event.window.data2,
            });
        case SDL_WINDOWEVENT_MINIMIZED:
            return detail::callEventCallbacks(
                events::WindowMinimized { .windowId = event.window.windowID });
        case SDL_WINDOWEVENT_MAXIMIZED:
            return detail::callEventCallbacks(
                events::WindowMaximized { .windowId = event.window.windowID });
        case SDL_WINDOWEVENT_RESTORED:
            return detail::callEventCallbacks(
                events::WindowRestored { .windowId = event.window.windowID });
        case SDL_WINDOWEVENT_ENTER:
            return detail::callEventCallbacks(
                events::WindowEnter { .windowId = event.window.windowID });
        case SDL_WINDOWEVENT_LEAVE:
            return detail::callEventCallbacks(
                events::WindowLeave { .windowId = event.window.windowID });
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            return detail::callEventCallbacks(
                events::WindowFocusGained { .windowId = event.window.windowID });
        case SDL_WINDOWEVENT_FOCUS_LOST:
            return detail::callEventCallbacks(
                events::WindowFocusLost { .windowId = event.window.windowID });
        case SDL_WINDOWEVENT_CLOSE:
            return detail::callEventCallbacks(
                events::WindowClose { .windowId = event.window.windowID });
        case SDL_WINDOWEVENT_TAKE_FOCUS:
            return std::nullopt;
        case SDL_WINDOWEVENT_HIT_TEST:
            return std::nullopt;
        }
    case SDL_SYSWMEVENT:
        break;
    case SDL_KEYDOWN:
        return detail::callEventCallbacks(events::KeyDown {
            .windowId = event.key.windowID,
            .state = static_cast<ButtonState>(event.key.state),
            .repeat = event.key.repeat > 0,
            .key = KeySymbol {
                .scancode = static_cast<Scancode>(event.key.keysym.scancode),
                .symbol = static_cast<Keycode>(event.key.keysym.sym),
                .mod = KeyModifiers::fromMask(event.key.keysym.mod),
            },
        });
    case SDL_KEYUP:
        return detail::callEventCallbacks(events::KeyUp {
            .windowId = event.key.windowID,
            .state = static_cast<ButtonState>(event.key.state),
            .repeat = event.key.repeat > 0,
            .key = KeySymbol {
                .scancode = static_cast<Scancode>(event.key.keysym.scancode),
                .symbol = static_cast<Keycode>(event.key.keysym.sym),
                .mod = KeyModifiers::fromMask(event.key.keysym.mod),
            },
        });
    case SDL_TEXTEDITING:
        return std::nullopt;
    case SDL_TEXTINPUT:
        return std::nullopt;
    case SDL_KEYMAPCHANGED:
        return std::nullopt;
    case SDL_TEXTEDITING_EXT:
        return std::nullopt;
    case SDL_MOUSEMOTION:
        return detail::callEventCallbacks(events::MouseMotion {
            .windowId = event.motion.windowID,
            .mouseInstanceId = event.motion.which,
            .state = MouseButtonState::fromMask(event.motion.state),
            .x = event.motion.x,
            .y = event.motion.y,
            .xRel = event.motion.xrel,
            .yRel = event.motion.yrel,
        });
    case SDL_MOUSEBUTTONDOWN:
        return detail::callEventCallbacks(events::MouseButtonDown {
            .windowId = event.button.windowID,
            .mouseInstanceId = event.button.which,
            .button = static_cast<MouseButton>(event.button.button),
            .state = static_cast<ButtonState>(event.button.state),
            .clicks = event.button.clicks,
            .x = event.button.x,
            .y = event.button.y,
        });
    case SDL_MOUSEBUTTONUP:
        return detail::callEventCallbacks(events::MouseButtonUp {
            .windowId = event.button.windowID,
            .mouseInstanceId = event.button.which,
            .button = static_cast<MouseButton>(event.button.button),
            .state = static_cast<ButtonState>(event.button.state),
            .clicks = event.button.clicks,
            .x = event.button.x,
            .y = event.button.y,
        });
    case SDL_MOUSEWHEEL:
        return detail::callEventCallbacks(events::MouseWheel {
            .windowId = event.wheel.windowID,
            .mouseInstanceId = event.wheel.which,
            .x = event.wheel.x,
            .y = event.wheel.y,
            .direction = static_cast<events::MouseWheel::Direction>(event.wheel.direction),
            .preciseX = event.wheel.preciseX,
            .preciseY = event.wheel.preciseY,
        });
    case SDL_JOYAXISMOTION: {
        auto joystick = getJoystick(event.jaxis.which);
        if (joystick) {
            return detail::callEventCallbacks(events::JoystickAxisMoved {
                .joystick = std::move(joystick),
                .axis = event.jaxis.axis,
                .value = axisToFloat(event.jaxis.value),
            });
        }
        break;
    }
    case SDL_JOYBALLMOTION:
        return std::nullopt;
    case SDL_JOYHATMOTION:
        return std::nullopt;
    case SDL_JOYBUTTONDOWN: {
        auto joystick = getJoystick(event.jbutton.which);
        if (joystick) {
            return detail::callEventCallbacks(events::JoystickButtonDown {
                .joystick = std::move(joystick),
                .button = event.jbutton.button,
                .state = static_cast<ButtonState>(event.jbutton.state),
            });
        }
        break;
    }
    case SDL_JOYBUTTONUP: {
        auto joystick = getJoystick(event.jbutton.which);
        if (joystick) {
            return detail::callEventCallbacks(events::JoystickButtonUp {
                .joystick = std::move(joystick),
                .button = event.jbutton.button,
                .state = static_cast<ButtonState>(event.jbutton.state),
            });
        }
        break;
    }
    // case SDL_CONTROLLERDEVICEADDED:
    // case SDL_CONTROLLERDEVICEREMOVED:
    case SDL_JOYDEVICEADDED: {
        auto joystick = addJoystick(event.jdevice.which);
        if (joystick) {
            return detail::callEventCallbacks(
                events::JoystickAdded { .joystick = std::move(joystick) });
        }
        break;
    }
    case SDL_JOYDEVICEREMOVED: {
        auto joystick = removeJoystick(event.jdevice.which);
        if (joystick) {
            return detail::callEventCallbacks(
                events::JoystickRemoved { .joystick = std::move(joystick) });
        }
        break;
    }
    case SDL_JOYBATTERYUPDATED:
        return std::nullopt;
    case SDL_CONTROLLERAXISMOTION: {
        auto joystick = getJoystick(event.caxis.which);
        if (joystick) {
            return detail::callEventCallbacks(events::ControllerAxisMoved {
                .joystick = std::move(joystick),
                .axis = static_cast<ControllerAxis>(event.caxis.axis),
                .value = axisToFloat(event.caxis.value),
            });
        }
        break;
    }
    case SDL_CONTROLLERBUTTONDOWN: {
        auto joystick = getJoystick(event.cbutton.which);
        if (joystick) {
            return detail::callEventCallbacks(events::ControllerButtonDown {
                .joystick = std::move(joystick),
                .button = static_cast<ControllerButton>(event.cbutton.button),
                .state = static_cast<ButtonState>(event.cbutton.state),
            });
        }
        break;
    }
    case SDL_CONTROLLERBUTTONUP: {
        auto joystick = getJoystick(event.cbutton.which);
        if (joystick) {
            return detail::callEventCallbacks(events::ControllerButtonUp {
                .joystick = std::move(joystick),
                .button = static_cast<ControllerButton>(event.cbutton.button),
                .state = static_cast<ButtonState>(event.cbutton.state),
            });
        }
        break;
    }
    case SDL_CONTROLLERDEVICEREMAPPED: {
        auto joystick = removeJoystick(event.cdevice.which);
        if (joystick) {
            return detail::callEventCallbacks(
                events::ControllerRemapped { .joystick = std::move(joystick) });
        }
        break;
    }
    case SDL_CONTROLLERTOUCHPADDOWN:
        return std::nullopt;
    case SDL_CONTROLLERTOUCHPADMOTION:
        return std::nullopt;
    case SDL_CONTROLLERTOUCHPADUP:
        return std::nullopt;
    case SDL_CONTROLLERSENSORUPDATE:
        return std::nullopt;
    case SDL_FINGERDOWN:
        return std::nullopt;
    case SDL_FINGERUP:
        return std::nullopt;
    case SDL_FINGERMOTION:
        return std::nullopt;
    case SDL_DOLLARGESTURE:
        return std::nullopt;
    case SDL_DOLLARRECORD:
        return std::nullopt;
    case SDL_MULTIGESTURE:
        return std::nullopt;
    case SDL_CLIPBOARDUPDATE:
        return std::nullopt;
    case SDL_DROPFILE:
        return std::nullopt;
    case SDL_DROPTEXT:
        return std::nullopt;
    case SDL_DROPBEGIN:
        return std::nullopt;
    case SDL_DROPCOMPLETE:
        return std::nullopt;
    case SDL_AUDIODEVICEADDED:
        return std::nullopt;
    case SDL_AUDIODEVICEREMOVED:
        return std::nullopt;
    case SDL_SENSORUPDATE:
        return std::nullopt;
    case SDL_RENDER_TARGETS_RESET:
        return std::nullopt;
    case SDL_RENDER_DEVICE_RESET:
        return std::nullopt;
    default:
        return std::nullopt;
    }
    return std::nullopt;
}

bool showMessageBox(
    MessageBoxType type, const std::string& title, const std::string& message, SDL_Window* window)
{
    return SDL_ShowSimpleMessageBox(
               static_cast<Uint32>(type), title.c_str(), message.c_str(), window)
        == 0;
}

std::optional<int> showMessageBox(const MessageBoxData& data)
{
    int pressedButtonId = 0;
    std::vector<SDL_MessageBoxButtonData> buttons;
    for (const auto& button : data.buttons) {
        buttons.push_back(SDL_MessageBoxButtonData {
            .flags = static_cast<Uint32>(button.flags),
            .buttonid = button.id,
            .text = button.text.c_str(),
        });
    }
    SDL_MessageBoxData mboxData {
        .flags = static_cast<Uint32>(data.type),
        .window = data.window,
        .title = data.title.c_str(),
        .message = data.message.c_str(),
        .numbuttons = static_cast<int>(buttons.size()),
        .buttons = buttons.data(),
        .colorScheme = nullptr,
    };
    const auto res = SDL_ShowMessageBox(&mboxData, &pressedButtonId);
    if (res != 0) {
        return std::nullopt;
    }
    return pressedButtonId;
}

std::string getError()
{
    return SDL_GetError();
}

std::string getPlatform()
{
    return SDL_GetPlatform();
}

bool openUrl(const std::string& url)
{
    return SDL_OpenURL(url.c_str()) == 0;
}

std::string getClipboardText()
{
    auto p = SDL_GetClipboardText();
    auto str = std::string(p);
    SDL_free(p);
    return str;
}

bool setClipboardText(const std::string& str)
{
    return SDL_SetClipboardText(str.c_str()) == 0;
}

float getTime()
{
    static const auto start = SDL_GetPerformanceCounter();
    const auto now = SDL_GetPerformanceCounter();
    return static_cast<float>(now - start) / SDL_GetPerformanceFrequency();
}

std::optional<Window> Window::create(
    const std::string& title, uint32_t width, uint32_t height, const WindowProperties& props)
{
    Window window;
    if (!window.init(title, width, height, props)) {
        return std::nullopt;
    }
    return window;
}

Window::~Window()
{
    destroy();
}

void Window::destroy()
{
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
}

Window::Window(Window&& other)
    : window_(std::exchange(other.window_, nullptr))
{
}

Window& Window::operator=(Window&& other)
{
    destroy();
    window_ = std::exchange(other.window_, nullptr);
    return *this;
}

Window::Size Window::getSize() const
{
    int w = 0, h = 0;
    SDL_GetWindowSize(window_, &w, &h);
    return { w, h };
}

void Window::maximize() const
{
    SDL_MaximizeWindow(window_);
}

void Window::setTitle(const std::string& title) const
{
    SDL_SetWindowTitle(window_, title.c_str());
}

SDL_Window* Window::getSdlWindow() const
{
    return window_;
}

void* Window::getNativeWindowHandle()
{
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(window_, &wmi)) {
        return nullptr;
    }
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
    if (wmi.info.win.window) {
        return wmi.info.win.window;
    }
#endif
#if defined(SDL_VIDEO_DRIVER_X11)
    if (wmi.info.x11.window) {
        return reinterpret_cast<void*>(wmi.info.x11.window);
    }
#endif
#if defined(SDL_VIDEO_DRIVER_COCOA)
    if (wmi.info.cocoa.window) {
        return wmi.info.cocoa.window;
    }
#endif
#if defined(SDL_VIDEO_DRIVER_WAYLAND)
    // TODO!
#endif
#if defined(SDL_VIDEO_DRIVER_ANDROID)
    if (wmi.info.android.window) {
        return wmi.info.android.window;
    }
#endif
    return nullptr;
}

void* Window::getNativeDisplayHandle()
{
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(window_, &wmi)) {
        return nullptr;
    }
#if defined(SDL_VIDEO_DRIVER_X11)
    if (wmi.info.x11.display) {
        return wmi.info.x11.display;
    }
#endif
#if defined(SDL_VIDEO_DRIVER_WAYLAND)
    if (wmi.info.wl.display) {
        return wmi.info.wl.display;
    }
#endif
    return nullptr;
}

bool Window::init(
    const std::string& title, uint32_t width, uint32_t height, const WindowProperties& props)
{
    uint32_t flags = 0;
    flags |= (props.openGl ? SDL_WINDOW_OPENGL : 0);
    const auto fs = props.fullscreen;
    flags |= (fs == FullscreenType::Fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
    flags |= (fs == FullscreenType::FullscreenDesktop ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    flags |= (props.hidden ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN);
    flags |= (props.borderless ? SDL_WINDOW_BORDERLESS : 0);
    flags |= (props.resizable ? SDL_WINDOW_RESIZABLE : 0);
    flags |= (props.maximized ? SDL_WINDOW_MAXIMIZED : 0);
    flags |= (props.minimized ? SDL_WINDOW_MINIMIZED : 0);
    flags |= (props.allowHighDpi ? SDL_WINDOW_ALLOW_HIGHDPI : 0);

    window_ = SDL_CreateWindow(
        title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags);
    if (!window_) {
        return false;
    }
    return true;
}

std::optional<GlWindow> GlWindow::create(const std::string& title, uint32_t width, uint32_t height,
    const WindowProperties& windowProps, const ContextProperties& contextProps)
{
    GlWindow window;
    if (!window.init(title, width, height, windowProps, contextProps)) {
        return std::nullopt;
    }
    return window;
}

GlWindow::~GlWindow()
{
    destroy();
}

void GlWindow::destroy()
{
    if (ctx_) {
        SDL_GL_DeleteContext(ctx_);
        ctx_ = nullptr;
    }
}

GlWindow::GlWindow(GlWindow&& other)
    : Window(std::move(other))
    , ctx_(std::exchange(other.ctx_, nullptr))
{
}

GlWindow& GlWindow::operator=(GlWindow&& other)
{
    Window::operator=(std::move(other));
    destroy();
    ctx_ = std::exchange(other.ctx_, nullptr);
    return *this;
}

void GlWindow::swap() const
{
    SDL_GL_SwapWindow(window_);
}

bool GlWindow::setSwapInterval(int interval) const
{
    return SDL_GL_SetSwapInterval(interval) == 0;
}

SDL_GLContext GlWindow::getSdlGlContext() const
{
    return ctx_;
}

bool GlWindow::init(const std::string& title, uint32_t width, uint32_t height,
    const WindowProperties& windowProps, const ContextProperties& contextProps)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, contextProps.contextMajor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, contextProps.contextMinor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, contextProps.redSize);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, contextProps.greenSize);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, contextProps.blueSize);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, contextProps.alphaSize);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, contextProps.stencil ? 8 : 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, contextProps.depth);

    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, contextProps.srgb ? 1 : 0);

#ifndef NDEBUG
    int contextFlags = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &contextFlags);
    contextFlags |= SDL_GL_CONTEXT_DEBUG_FLAG;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, contextFlags);
#endif

    if (contextProps.msaaSamples > 0) {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, contextProps.msaaSamples);
    }

    auto oglProps = windowProps;
    oglProps.openGl = true;
    if (!Window::init(title, width, height, oglProps)) {
        return false;
    }

    ctx_ = SDL_GL_CreateContext(window_);
    if (!ctx_) {
        return false;
    }

    return true;
}
}
