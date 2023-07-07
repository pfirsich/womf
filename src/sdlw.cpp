#include "sdlw.hpp"

#include <utility>

#include <SDL2/SDL_syswm.h>

namespace {
bool initSubSystem(uint32_t flags)
{
    if (SDL_WasInit(flags) == 0) {
        if (SDL_InitSubSystem(flags) < 0) {
            return false;
        }
    }
    return true;
}
}

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
    static auto state = SDL_GetKeyboardState(&numKeys);
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
    case SDL_JOYAXISMOTION:
        return std::nullopt;
    case SDL_JOYBALLMOTION:
        return std::nullopt;
    case SDL_JOYHATMOTION:
        return std::nullopt;
    case SDL_JOYBUTTONDOWN:
        return std::nullopt;
    case SDL_JOYBUTTONUP:
        return std::nullopt;
    case SDL_JOYDEVICEADDED:
        return std::nullopt;
    case SDL_JOYDEVICEREMOVED:
        return std::nullopt;
    case SDL_JOYBATTERYUPDATED:
        return std::nullopt;
    case SDL_CONTROLLERAXISMOTION:
        return std::nullopt;
    case SDL_CONTROLLERBUTTONDOWN:
        return std::nullopt;
    case SDL_CONTROLLERBUTTONUP:
        return std::nullopt;
    case SDL_CONTROLLERDEVICEADDED:
        return std::nullopt;
    case SDL_CONTROLLERDEVICEREMOVED:
        return std::nullopt;
    case SDL_CONTROLLERDEVICEREMAPPED:
        return std::nullopt;
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
    if (!initSubSystem(SDL_INIT_TIMER))
        return 0.0f;
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
    if (Sdl::instance().getResult() < 0) {
        return false;
    }

    if (!initSubSystem(SDL_INIT_VIDEO)) {
        return false;
    }

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
    if (Sdl::instance().getResult() < 0) {
        return false;
    }

    if (!initSubSystem(SDL_INIT_VIDEO)) {
        return false;
    }

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
