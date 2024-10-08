#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <SDL.h>

// Windows, you are drunk. Go home.
#undef near
#undef far

/* TODO:
 * - Text Input: https://wiki.libsdl.org/SDL2/Tutorials-TextInput
 * - All the joystick & gamepad shit
 */

namespace sdlw {
// This only includes the one I currently support
enum class SubSystem : uint32_t {
    Timer = SDL_INIT_TIMER,
    Video = SDL_INIT_VIDEO,
    Joystick = SDL_INIT_JOYSTICK,
    GameController = SDL_INIT_GAMECONTROLLER,
    Events = SDL_INIT_EVENTS,
    Everything = SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER
        | SDL_INIT_EVENTS,
};

class Sdl {
public:
    template <typename... Args>
    Sdl(Args&&... systems)
        : flags_((... | static_cast<uint32_t>(systems)))
    {
        result_ = SDL_InitSubSystem(flags_);
    }

    ~Sdl() { SDL_QuitSubSystem(flags_); }

    operator int() const { return result_; }

private:
    Sdl(const Sdl&) = delete;
    Sdl(Sdl&&) = delete;
    Sdl& operator=(const Sdl&) = delete;
    Sdl& operator=(Sdl&&) = delete;

    // SDL_Init returns 0 on success and < 0 on error
    uint32_t flags_ = 0;
    int result_ = -1;
};

enum class ButtonState {
    Pressed = SDL_PRESSED,
    Released = SDL_RELEASED,
};

enum class JoystickHatPosition {
    LeftUp = SDL_HAT_LEFTUP,
    Up = SDL_HAT_UP,
    RightUp = SDL_HAT_RIGHTUP,
    Left = SDL_HAT_LEFT,
    Centered = SDL_HAT_CENTERED,
    Right = SDL_HAT_RIGHT,
    LeftDown = SDL_HAT_LEFTDOWN,
    Down = SDL_HAT_DOWN,
    RightDown = SDL_HAT_RIGHTDOWN,
};

enum class MouseButton {
    Left = SDL_BUTTON_LEFT,
    Middle = SDL_BUTTON_MIDDLE,
    Right = SDL_BUTTON_RIGHT,
    X1 = SDL_BUTTON_X1,
    X2 = SDL_BUTTON_X2,
};

struct MouseButtonState {
    bool left;
    bool middle;
    bool right;
    bool x1;
    bool x2;

    static MouseButtonState fromMask(uint32_t mask)
    {
        return MouseButtonState {
            .left = (mask & SDL_BUTTON_LMASK) > 0,
            .middle = (mask & SDL_BUTTON_MMASK) > 0,
            .right = (mask & SDL_BUTTON_RMASK) > 0,
            .x1 = (mask & SDL_BUTTON_X1MASK) > 0,
            .x2 = (mask & SDL_BUTTON_X2MASK) > 0,
        };
    }
};

struct KeyModifiers {
    bool lshift;
    bool rshift;
    bool lctrl;
    bool rctrl;
    bool lalt;
    bool ralt;
    bool lgui;
    bool rgui;
    bool num;
    bool caps;
    bool mode;
    bool scroll;
    bool ctrl;
    bool shift;
    bool alt;
    bool gui;

    static KeyModifiers fromMask(uint16_t mask)
    {
        return KeyModifiers {
            .lshift = (mask & KMOD_LSHIFT) > 0,
            .rshift = (mask & KMOD_RSHIFT) > 0,
            .lctrl = (mask & KMOD_LCTRL) > 0,
            .rctrl = (mask & KMOD_RCTRL) > 0,
            .lalt = (mask & KMOD_LALT) > 0,
            .ralt = (mask & KMOD_RALT) > 0,
            .lgui = (mask & KMOD_LGUI) > 0,
            .rgui = (mask & KMOD_RGUI) > 0,
            .num = (mask & KMOD_NUM) > 0,
            .caps = (mask & KMOD_CAPS) > 0,
            .mode = (mask & KMOD_MODE) > 0,
            .scroll = (mask & KMOD_SCROLL) > 0,
            .ctrl = (mask & KMOD_CTRL) > 0,
            .shift = (mask & KMOD_SHIFT) > 0,
            .alt = (mask & KMOD_ALT) > 0,
            .gui = (mask & KMOD_GUI) > 0,
        };
    }
};

enum class Scancode {
    Unknown = SDL_SCANCODE_UNKNOWN,
    A = SDL_SCANCODE_A,
    B = SDL_SCANCODE_B,
    C = SDL_SCANCODE_C,
    D = SDL_SCANCODE_D,
    E = SDL_SCANCODE_E,
    F = SDL_SCANCODE_F,
    G = SDL_SCANCODE_G,
    H = SDL_SCANCODE_H,
    I = SDL_SCANCODE_I,
    J = SDL_SCANCODE_J,
    K = SDL_SCANCODE_K,
    L = SDL_SCANCODE_L,
    M = SDL_SCANCODE_M,
    N = SDL_SCANCODE_N,
    O = SDL_SCANCODE_O,
    P = SDL_SCANCODE_P,
    Q = SDL_SCANCODE_Q,
    R = SDL_SCANCODE_R,
    S = SDL_SCANCODE_S,
    T = SDL_SCANCODE_T,
    U = SDL_SCANCODE_U,
    V = SDL_SCANCODE_V,
    W = SDL_SCANCODE_W,
    X = SDL_SCANCODE_X,
    Y = SDL_SCANCODE_Y,
    Z = SDL_SCANCODE_Z,
    N1 = SDL_SCANCODE_1,
    N2 = SDL_SCANCODE_2,
    N3 = SDL_SCANCODE_3,
    N4 = SDL_SCANCODE_4,
    N5 = SDL_SCANCODE_5,
    N6 = SDL_SCANCODE_6,
    N7 = SDL_SCANCODE_7,
    N8 = SDL_SCANCODE_8,
    N9 = SDL_SCANCODE_9,
    N0 = SDL_SCANCODE_0,
    Return = SDL_SCANCODE_RETURN,
    Escape = SDL_SCANCODE_ESCAPE,
    Backspace = SDL_SCANCODE_BACKSPACE,
    Tab = SDL_SCANCODE_TAB,
    Space = SDL_SCANCODE_SPACE,
    Minus = SDL_SCANCODE_MINUS,
    Equals = SDL_SCANCODE_EQUALS,
    LeftBracket = SDL_SCANCODE_LEFTBRACKET,
    RightBracket = SDL_SCANCODE_RIGHTBRACKET,
    Backslash = SDL_SCANCODE_BACKSLASH,
    NonUsHash = SDL_SCANCODE_NONUSHASH,
    Semicolon = SDL_SCANCODE_SEMICOLON,
    Apostrophe = SDL_SCANCODE_APOSTROPHE,
    Grave = SDL_SCANCODE_GRAVE,
    Comma = SDL_SCANCODE_COMMA,
    Period = SDL_SCANCODE_PERIOD,
    Slash = SDL_SCANCODE_SLASH,
    Capslock = SDL_SCANCODE_CAPSLOCK,
    F1 = SDL_SCANCODE_F1,
    F2 = SDL_SCANCODE_F2,
    F3 = SDL_SCANCODE_F3,
    F4 = SDL_SCANCODE_F4,
    F5 = SDL_SCANCODE_F5,
    F6 = SDL_SCANCODE_F6,
    F7 = SDL_SCANCODE_F7,
    F8 = SDL_SCANCODE_F8,
    F9 = SDL_SCANCODE_F9,
    F10 = SDL_SCANCODE_F10,
    F11 = SDL_SCANCODE_F11,
    F12 = SDL_SCANCODE_F12,
    PrintScreen = SDL_SCANCODE_PRINTSCREEN,
    ScrollLock = SDL_SCANCODE_SCROLLLOCK,
    Pause = SDL_SCANCODE_PAUSE,
    Insert = SDL_SCANCODE_INSERT,
    Home = SDL_SCANCODE_HOME,
    PageUp = SDL_SCANCODE_PAGEUP,
    Delete = SDL_SCANCODE_DELETE,
    End = SDL_SCANCODE_END,
    PageDown = SDL_SCANCODE_PAGEDOWN,
    Right = SDL_SCANCODE_RIGHT,
    Left = SDL_SCANCODE_LEFT,
    Down = SDL_SCANCODE_DOWN,
    Up = SDL_SCANCODE_UP,
    NumLockClear = SDL_SCANCODE_NUMLOCKCLEAR,
    KpDivide = SDL_SCANCODE_KP_DIVIDE,
    KpMultiply = SDL_SCANCODE_KP_MULTIPLY,
    KpMinus = SDL_SCANCODE_KP_MINUS,
    KpPlus = SDL_SCANCODE_KP_PLUS,
    KpEnter = SDL_SCANCODE_KP_ENTER,
    Kp1 = SDL_SCANCODE_KP_1,
    Kp2 = SDL_SCANCODE_KP_2,
    Kp3 = SDL_SCANCODE_KP_3,
    Kp4 = SDL_SCANCODE_KP_4,
    Kp5 = SDL_SCANCODE_KP_5,
    Kp6 = SDL_SCANCODE_KP_6,
    Kp7 = SDL_SCANCODE_KP_7,
    Kp8 = SDL_SCANCODE_KP_8,
    Kp9 = SDL_SCANCODE_KP_9,
    Kp0 = SDL_SCANCODE_KP_0,
    KpPeriod = SDL_SCANCODE_KP_PERIOD,
    NonUsBackslash = SDL_SCANCODE_NONUSBACKSLASH,
    Application = SDL_SCANCODE_APPLICATION,
    Power = SDL_SCANCODE_POWER,
    KpEquals = SDL_SCANCODE_KP_EQUALS,
    F13 = SDL_SCANCODE_F13,
    F14 = SDL_SCANCODE_F14,
    F15 = SDL_SCANCODE_F15,
    F16 = SDL_SCANCODE_F16,
    F17 = SDL_SCANCODE_F17,
    F18 = SDL_SCANCODE_F18,
    F19 = SDL_SCANCODE_F19,
    F20 = SDL_SCANCODE_F20,
    F21 = SDL_SCANCODE_F21,
    F22 = SDL_SCANCODE_F22,
    F23 = SDL_SCANCODE_F23,
    F24 = SDL_SCANCODE_F24,
    Execute = SDL_SCANCODE_EXECUTE,
    Help = SDL_SCANCODE_HELP,
    Menu = SDL_SCANCODE_MENU,
    Select = SDL_SCANCODE_SELECT,
    Stop = SDL_SCANCODE_STOP,
    Again = SDL_SCANCODE_AGAIN,
    Undo = SDL_SCANCODE_UNDO,
    Cut = SDL_SCANCODE_CUT,
    Copy = SDL_SCANCODE_COPY,
    Paste = SDL_SCANCODE_PASTE,
    Find = SDL_SCANCODE_FIND,
    Mute = SDL_SCANCODE_MUTE,
    VolumeUp = SDL_SCANCODE_VOLUMEUP,
    VolumeDown = SDL_SCANCODE_VOLUMEDOWN,
    KpComma = SDL_SCANCODE_KP_COMMA,
    KpEqualsAs400 = SDL_SCANCODE_KP_EQUALSAS400,
    International1 = SDL_SCANCODE_INTERNATIONAL1,
    International2 = SDL_SCANCODE_INTERNATIONAL2,
    International3 = SDL_SCANCODE_INTERNATIONAL3,
    International4 = SDL_SCANCODE_INTERNATIONAL4,
    International5 = SDL_SCANCODE_INTERNATIONAL5,
    International6 = SDL_SCANCODE_INTERNATIONAL6,
    International7 = SDL_SCANCODE_INTERNATIONAL7,
    International8 = SDL_SCANCODE_INTERNATIONAL8,
    International9 = SDL_SCANCODE_INTERNATIONAL9,
    Lang1 = SDL_SCANCODE_LANG1,
    Lang2 = SDL_SCANCODE_LANG2,
    Lang3 = SDL_SCANCODE_LANG3,
    Lang4 = SDL_SCANCODE_LANG4,
    Lang5 = SDL_SCANCODE_LANG5,
    Lang6 = SDL_SCANCODE_LANG6,
    Lang7 = SDL_SCANCODE_LANG7,
    Lang8 = SDL_SCANCODE_LANG8,
    Lang9 = SDL_SCANCODE_LANG9,
    AltErase = SDL_SCANCODE_ALTERASE,
    SysReq = SDL_SCANCODE_SYSREQ,
    Cancel = SDL_SCANCODE_CANCEL,
    Clear = SDL_SCANCODE_CLEAR,
    Prior = SDL_SCANCODE_PRIOR,
    Return2 = SDL_SCANCODE_RETURN2,
    Separator = SDL_SCANCODE_SEPARATOR,
    Out = SDL_SCANCODE_OUT,
    Oper = SDL_SCANCODE_OPER,
    ClearAgain = SDL_SCANCODE_CLEARAGAIN,
    Crsel = SDL_SCANCODE_CRSEL,
    Exsel = SDL_SCANCODE_EXSEL,
    Kp00 = SDL_SCANCODE_KP_00,
    Kp000 = SDL_SCANCODE_KP_000,
    ThousandsSeparator = SDL_SCANCODE_THOUSANDSSEPARATOR,
    DecimalSeparator = SDL_SCANCODE_DECIMALSEPARATOR,
    CurrencyUnit = SDL_SCANCODE_CURRENCYUNIT,
    CurrencySubUnit = SDL_SCANCODE_CURRENCYSUBUNIT,
    KpLeftParen = SDL_SCANCODE_KP_LEFTPAREN,
    KpRightParen = SDL_SCANCODE_KP_RIGHTPAREN,
    KpLeftBrace = SDL_SCANCODE_KP_LEFTBRACE,
    KpRightBrace = SDL_SCANCODE_KP_RIGHTBRACE,
    KpTab = SDL_SCANCODE_KP_TAB,
    KpBackspace = SDL_SCANCODE_KP_BACKSPACE,
    KpA = SDL_SCANCODE_KP_A,
    KpB = SDL_SCANCODE_KP_B,
    KpC = SDL_SCANCODE_KP_C,
    KpD = SDL_SCANCODE_KP_D,
    KpE = SDL_SCANCODE_KP_E,
    KpF = SDL_SCANCODE_KP_F,
    KpXor = SDL_SCANCODE_KP_XOR,
    KpPower = SDL_SCANCODE_KP_POWER,
    KpPercent = SDL_SCANCODE_KP_PERCENT,
    KpLess = SDL_SCANCODE_KP_LESS,
    KpGreater = SDL_SCANCODE_KP_GREATER,
    KpAmpersand = SDL_SCANCODE_KP_AMPERSAND,
    KpDblampersand = SDL_SCANCODE_KP_DBLAMPERSAND,
    KpVerticalbar = SDL_SCANCODE_KP_VERTICALBAR,
    KpDblverticalbar = SDL_SCANCODE_KP_DBLVERTICALBAR,
    KpColon = SDL_SCANCODE_KP_COLON,
    KpHash = SDL_SCANCODE_KP_HASH,
    KpSpace = SDL_SCANCODE_KP_SPACE,
    KpAt = SDL_SCANCODE_KP_AT,
    KpExclam = SDL_SCANCODE_KP_EXCLAM,
    KpMemstore = SDL_SCANCODE_KP_MEMSTORE,
    KpMemrecall = SDL_SCANCODE_KP_MEMRECALL,
    KpMemclear = SDL_SCANCODE_KP_MEMCLEAR,
    KpMemadd = SDL_SCANCODE_KP_MEMADD,
    KpMemsubtract = SDL_SCANCODE_KP_MEMSUBTRACT,
    KpMemmultiply = SDL_SCANCODE_KP_MEMMULTIPLY,
    KpMemdivide = SDL_SCANCODE_KP_MEMDIVIDE,
    KpPlusminus = SDL_SCANCODE_KP_PLUSMINUS,
    KpClear = SDL_SCANCODE_KP_CLEAR,
    KpClearentry = SDL_SCANCODE_KP_CLEARENTRY,
    KpBinary = SDL_SCANCODE_KP_BINARY,
    KpOctal = SDL_SCANCODE_KP_OCTAL,
    KpDecimal = SDL_SCANCODE_KP_DECIMAL,
    KpHexadecimal = SDL_SCANCODE_KP_HEXADECIMAL,
    Lctrl = SDL_SCANCODE_LCTRL,
    Lshift = SDL_SCANCODE_LSHIFT,
    Lalt = SDL_SCANCODE_LALT,
    Lgui = SDL_SCANCODE_LGUI,
    Rctrl = SDL_SCANCODE_RCTRL,
    Rshift = SDL_SCANCODE_RSHIFT,
    Ralt = SDL_SCANCODE_RALT,
    Rgui = SDL_SCANCODE_RGUI,
    Mode = SDL_SCANCODE_MODE,
    AudioNext = SDL_SCANCODE_AUDIONEXT,
    AudioPrev = SDL_SCANCODE_AUDIOPREV,
    AudioStop = SDL_SCANCODE_AUDIOSTOP,
    AudioPlay = SDL_SCANCODE_AUDIOPLAY,
    AudioMute = SDL_SCANCODE_AUDIOMUTE,
    MediaSelect = SDL_SCANCODE_MEDIASELECT,
    Www = SDL_SCANCODE_WWW,
    Mail = SDL_SCANCODE_MAIL,
    Calculator = SDL_SCANCODE_CALCULATOR,
    Computer = SDL_SCANCODE_COMPUTER,
    AcSearch = SDL_SCANCODE_AC_SEARCH,
    AcHome = SDL_SCANCODE_AC_HOME,
    AcBack = SDL_SCANCODE_AC_BACK,
    AcForward = SDL_SCANCODE_AC_FORWARD,
    AcStop = SDL_SCANCODE_AC_STOP,
    AcRefresh = SDL_SCANCODE_AC_REFRESH,
    AcBookmarks = SDL_SCANCODE_AC_BOOKMARKS,
    BrightnessDown = SDL_SCANCODE_BRIGHTNESSDOWN,
    BrightnessUp = SDL_SCANCODE_BRIGHTNESSUP,
    DisplaySwitch = SDL_SCANCODE_DISPLAYSWITCH,
    KbdIllumToggle = SDL_SCANCODE_KBDILLUMTOGGLE,
    KbdIllumDown = SDL_SCANCODE_KBDILLUMDOWN,
    KbdIllumUp = SDL_SCANCODE_KBDILLUMUP,
    Eject = SDL_SCANCODE_EJECT,
    Sleep = SDL_SCANCODE_SLEEP,
    App1 = SDL_SCANCODE_APP1,
    App2 = SDL_SCANCODE_APP2,
    AudioRewind = SDL_SCANCODE_AUDIOREWIND,
    AudioFastForward = SDL_SCANCODE_AUDIOFASTFORWARD,
    SoftLeft = SDL_SCANCODE_SOFTLEFT,
    SoftRight = SDL_SCANCODE_SOFTRIGHT,
    Call = SDL_SCANCODE_CALL,
    Endcall = SDL_SCANCODE_ENDCALL,
};

enum class Keycode {
    Unknown = SDLK_UNKNOWN,
    Return = SDLK_RETURN,
    Escape = SDLK_ESCAPE,
    Backspace = SDLK_BACKSPACE,
    Tab = SDLK_TAB,
    Space = SDLK_SPACE,
    Exclaim = SDLK_EXCLAIM,
    Quotedbl = SDLK_QUOTEDBL,
    Hash = SDLK_HASH,
    Percent = SDLK_PERCENT,
    Dollar = SDLK_DOLLAR,
    Ampersand = SDLK_AMPERSAND,
    Quote = SDLK_QUOTE,
    LeftParen = SDLK_LEFTPAREN,
    RightParen = SDLK_RIGHTPAREN,
    Asterisk = SDLK_ASTERISK,
    Plus = SDLK_PLUS,
    Comma = SDLK_COMMA,
    Minus = SDLK_MINUS,
    Period = SDLK_PERIOD,
    Slash = SDLK_SLASH,
    N0 = SDLK_0,
    N1 = SDLK_1,
    N2 = SDLK_2,
    N3 = SDLK_3,
    N4 = SDLK_4,
    N5 = SDLK_5,
    N6 = SDLK_6,
    N7 = SDLK_7,
    N8 = SDLK_8,
    N9 = SDLK_9,
    Colon = SDLK_COLON,
    Semicolon = SDLK_SEMICOLON,
    Less = SDLK_LESS,
    Equals = SDLK_EQUALS,
    Greater = SDLK_GREATER,
    Question = SDLK_QUESTION,
    At = SDLK_AT,
    LeftBracket = SDLK_LEFTBRACKET,
    Backslash = SDLK_BACKSLASH,
    RightBracket = SDLK_RIGHTBRACKET,
    Caret = SDLK_CARET,
    Underscore = SDLK_UNDERSCORE,
    Backquote = SDLK_BACKQUOTE,
    A = SDLK_a,
    B = SDLK_b,
    C = SDLK_c,
    D = SDLK_d,
    E = SDLK_e,
    F = SDLK_f,
    G = SDLK_g,
    H = SDLK_h,
    I = SDLK_i,
    J = SDLK_j,
    K = SDLK_k,
    L = SDLK_l,
    M = SDLK_m,
    N = SDLK_n,
    O = SDLK_o,
    P = SDLK_p,
    Q = SDLK_q,
    R = SDLK_r,
    S = SDLK_s,
    T = SDLK_t,
    U = SDLK_u,
    V = SDLK_v,
    W = SDLK_w,
    X = SDLK_x,
    Y = SDLK_y,
    Z = SDLK_z,
    Capslock = SDLK_CAPSLOCK,
    F1 = SDLK_F1,
    F2 = SDLK_F2,
    F3 = SDLK_F3,
    F4 = SDLK_F4,
    F5 = SDLK_F5,
    F6 = SDLK_F6,
    F7 = SDLK_F7,
    F8 = SDLK_F8,
    F9 = SDLK_F9,
    F10 = SDLK_F10,
    F11 = SDLK_F11,
    F12 = SDLK_F12,
    Printscreen = SDLK_PRINTSCREEN,
    Scrolllock = SDLK_SCROLLLOCK,
    Pause = SDLK_PAUSE,
    Insert = SDLK_INSERT,
    Home = SDLK_HOME,
    PageUp = SDLK_PAGEUP,
    Delete = SDLK_DELETE,
    End = SDLK_END,
    PageDown = SDLK_PAGEDOWN,
    Right = SDLK_RIGHT,
    Left = SDLK_LEFT,
    Down = SDLK_DOWN,
    Up = SDLK_UP,
    NumLockClear = SDLK_NUMLOCKCLEAR,
    KpDivide = SDLK_KP_DIVIDE,
    KpMultiply = SDLK_KP_MULTIPLY,
    KpMinus = SDLK_KP_MINUS,
    KpPlus = SDLK_KP_PLUS,
    KpEnter = SDLK_KP_ENTER,
    Kp1 = SDLK_KP_1,
    Kp2 = SDLK_KP_2,
    Kp3 = SDLK_KP_3,
    Kp4 = SDLK_KP_4,
    Kp5 = SDLK_KP_5,
    Kp6 = SDLK_KP_6,
    Kp7 = SDLK_KP_7,
    Kp8 = SDLK_KP_8,
    Kp9 = SDLK_KP_9,
    Kp0 = SDLK_KP_0,
    KpPeriod = SDLK_KP_PERIOD,
    Application = SDLK_APPLICATION,
    Power = SDLK_POWER,
    KpEquals = SDLK_KP_EQUALS,
    F13 = SDLK_F13,
    F14 = SDLK_F14,
    F15 = SDLK_F15,
    F16 = SDLK_F16,
    F17 = SDLK_F17,
    F18 = SDLK_F18,
    F19 = SDLK_F19,
    F20 = SDLK_F20,
    F21 = SDLK_F21,
    F22 = SDLK_F22,
    F23 = SDLK_F23,
    F24 = SDLK_F24,
    Execute = SDLK_EXECUTE,
    Help = SDLK_HELP,
    Menu = SDLK_MENU,
    Select = SDLK_SELECT,
    Stop = SDLK_STOP,
    Again = SDLK_AGAIN,
    Undo = SDLK_UNDO,
    Cut = SDLK_CUT,
    Copy = SDLK_COPY,
    Paste = SDLK_PASTE,
    Find = SDLK_FIND,
    Mute = SDLK_MUTE,
    VolumeUp = SDLK_VOLUMEUP,
    VolumeDown = SDLK_VOLUMEDOWN,
    KpComma = SDLK_KP_COMMA,
    KpEqualsAs400 = SDLK_KP_EQUALSAS400,
    AltErase = SDLK_ALTERASE,
    Sysreq = SDLK_SYSREQ,
    Cancel = SDLK_CANCEL,
    Clear = SDLK_CLEAR,
    Prior = SDLK_PRIOR,
    Return2 = SDLK_RETURN2,
    Separator = SDLK_SEPARATOR,
    Out = SDLK_OUT,
    Oper = SDLK_OPER,
    ClearAgain = SDLK_CLEARAGAIN,
    Crsel = SDLK_CRSEL,
    Exsel = SDLK_EXSEL,
    Kp00 = SDLK_KP_00,
    Kp000 = SDLK_KP_000,
    ThousandsSeparator = SDLK_THOUSANDSSEPARATOR,
    DecimalSeparator = SDLK_DECIMALSEPARATOR,
    CurrencyUnit = SDLK_CURRENCYUNIT,
    CurrencySubUnit = SDLK_CURRENCYSUBUNIT,
    KpLeftParen = SDLK_KP_LEFTPAREN,
    KpRightParen = SDLK_KP_RIGHTPAREN,
    KpLeftBrace = SDLK_KP_LEFTBRACE,
    KpRightBrace = SDLK_KP_RIGHTBRACE,
    KpTab = SDLK_KP_TAB,
    KpBackspace = SDLK_KP_BACKSPACE,
    KpA = SDLK_KP_A,
    KpB = SDLK_KP_B,
    KpC = SDLK_KP_C,
    KpD = SDLK_KP_D,
    KpE = SDLK_KP_E,
    KpF = SDLK_KP_F,
    KpXor = SDLK_KP_XOR,
    KpPower = SDLK_KP_POWER,
    KpPercent = SDLK_KP_PERCENT,
    KpLess = SDLK_KP_LESS,
    KpGreater = SDLK_KP_GREATER,
    KpAmpersand = SDLK_KP_AMPERSAND,
    KpDblampersand = SDLK_KP_DBLAMPERSAND,
    KpVerticalbar = SDLK_KP_VERTICALBAR,
    KpDblverticalbar = SDLK_KP_DBLVERTICALBAR,
    KpColon = SDLK_KP_COLON,
    KpHash = SDLK_KP_HASH,
    KpSpace = SDLK_KP_SPACE,
    KpAt = SDLK_KP_AT,
    KpExclam = SDLK_KP_EXCLAM,
    KpMemstore = SDLK_KP_MEMSTORE,
    KpMemrecall = SDLK_KP_MEMRECALL,
    KpMemclear = SDLK_KP_MEMCLEAR,
    KpMemadd = SDLK_KP_MEMADD,
    KpMemsubtract = SDLK_KP_MEMSUBTRACT,
    KpMemmultiply = SDLK_KP_MEMMULTIPLY,
    KpMemdivide = SDLK_KP_MEMDIVIDE,
    KpPlusminus = SDLK_KP_PLUSMINUS,
    KpClear = SDLK_KP_CLEAR,
    KpClearentry = SDLK_KP_CLEARENTRY,
    KpBinary = SDLK_KP_BINARY,
    KpOctal = SDLK_KP_OCTAL,
    KpDecimal = SDLK_KP_DECIMAL,
    KpHexadecimal = SDLK_KP_HEXADECIMAL,
    Lctrl = SDLK_LCTRL,
    Lshift = SDLK_LSHIFT,
    Lalt = SDLK_LALT,
    Lgui = SDLK_LGUI,
    Rctrl = SDLK_RCTRL,
    Rshift = SDLK_RSHIFT,
    Ralt = SDLK_RALT,
    Rgui = SDLK_RGUI,
    Mode = SDLK_MODE,
    AudioNext = SDLK_AUDIONEXT,
    AudioPrev = SDLK_AUDIOPREV,
    AudioStop = SDLK_AUDIOSTOP,
    AudioPlay = SDLK_AUDIOPLAY,
    AudioMute = SDLK_AUDIOMUTE,
    MediaSelect = SDLK_MEDIASELECT,
    Www = SDLK_WWW,
    Mail = SDLK_MAIL,
    Calculator = SDLK_CALCULATOR,
    Computer = SDLK_COMPUTER,
    AcSearch = SDLK_AC_SEARCH,
    AcHome = SDLK_AC_HOME,
    AcBack = SDLK_AC_BACK,
    AcForward = SDLK_AC_FORWARD,
    AcStop = SDLK_AC_STOP,
    AcRefresh = SDLK_AC_REFRESH,
    AcBookmarks = SDLK_AC_BOOKMARKS,
    BrightnessDown = SDLK_BRIGHTNESSDOWN,
    BrightnessUp = SDLK_BRIGHTNESSUP,
    DisplaySwitch = SDLK_DISPLAYSWITCH,
    KbdIllumToggle = SDLK_KBDILLUMTOGGLE,
    KbdIllumDown = SDLK_KBDILLUMDOWN,
    KbdIllumUp = SDLK_KBDILLUMUP,
    Eject = SDLK_EJECT,
    Sleep = SDLK_SLEEP,
    App1 = SDLK_APP1,
    App2 = SDLK_APP2,
    AudioRewind = SDLK_AUDIOREWIND,
    AudioFastForward = SDLK_AUDIOFASTFORWARD,
    SoftLeft = SDLK_SOFTLEFT,
    SoftRight = SDLK_SOFTRIGHT,
    Call = SDLK_CALL,
    Endcall = SDLK_ENDCALL,
};

Keycode toKeycode(Scancode scancode);
Scancode toScancode(Keycode key);

bool isDown(Scancode scancode);
bool isDown(Keycode key);

MouseButtonState getMouseButtonState(uint32_t bitmask);
MouseButtonState getMouseButtonState();

struct MousePosition {
    int x;
    int y;
};

MousePosition getMousePosition();

bool setRelativeMouseMode(bool enabled);
bool getRelativeMouseMode();

bool setCursorVisible(bool visible);
bool isCursorVisible();

/*
 * The joystick/controller API is pretty different from the SDL2 API, because I don't like it very
 * much and because we have better ways to deal with ownership in C++.
 * Instead of having a hidden array of devices that I can then open as either joystick or
 * controller, I just have a function that gives you a bunch of joysticks, which are optionally also
 * controllers.
 */

enum class ControllerButton {
    Invalid = SDL_CONTROLLER_BUTTON_INVALID,
    A = SDL_CONTROLLER_BUTTON_A,
    B = SDL_CONTROLLER_BUTTON_B,
    X = SDL_CONTROLLER_BUTTON_X,
    Y = SDL_CONTROLLER_BUTTON_Y,
    Back = SDL_CONTROLLER_BUTTON_BACK,
    Guide = SDL_CONTROLLER_BUTTON_GUIDE,
    Start = SDL_CONTROLLER_BUTTON_START,
    LeftStick = SDL_CONTROLLER_BUTTON_LEFTSTICK,
    RightStick = SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    LeftShoulder = SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    RightShoulder = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    DpadUp = SDL_CONTROLLER_BUTTON_DPAD_UP,
    DpadDown = SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    DpadLeft = SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    DpadRight = SDL_CONTROLLER_BUTTON_DPAD_RIGHT,

    // Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon
    // Luna microphone button
    Misc1 = SDL_CONTROLLER_BUTTON_MISC1,
    Paddle1 = SDL_CONTROLLER_BUTTON_PADDLE1, // Xbox Elite paddle P1
    Paddle2 = SDL_CONTROLLER_BUTTON_PADDLE2, // Xbox Elite paddle P3
    Paddle3 = SDL_CONTROLLER_BUTTON_PADDLE3, // Xbox Elite paddle P2
    Paddle4 = SDL_CONTROLLER_BUTTON_PADDLE4, // Xbox Elite paddle P4
    Touchpad = SDL_CONTROLLER_BUTTON_TOUCHPAD, // PS4/PS5 touchpad button
};

std::string_view toString(ControllerButton button);
ControllerButton controllerButtonFromString(const std::string& str);

enum class ControllerAxis {
    Invalid = SDL_CONTROLLER_AXIS_INVALID,
    Leftx = SDL_CONTROLLER_AXIS_LEFTX,
    Lefty = SDL_CONTROLLER_AXIS_LEFTY,
    Rightx = SDL_CONTROLLER_AXIS_RIGHTX,
    Righty = SDL_CONTROLLER_AXIS_RIGHTY,
    LeftTrigger = SDL_CONTROLLER_AXIS_TRIGGERLEFT,
    RightTrigger = SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
};

std::string_view toString(ControllerAxis axis);
ControllerAxis controllerAxisFromString(const std::string& str);

enum class JoystickPowerLevel {
    Unknown = SDL_JOYSTICK_POWER_UNKNOWN,
    Empty = SDL_JOYSTICK_POWER_EMPTY, /* <= 5% */
    Low = SDL_JOYSTICK_POWER_LOW, /* <= 20% */
    Medium = SDL_JOYSTICK_POWER_MEDIUM, /* <= 70% */
    Full = SDL_JOYSTICK_POWER_FULL, /* <= 100% */
    Wired = SDL_JOYSTICK_POWER_WIRED,
};

enum class AddMappingResult {
    MappingAdded,
    MappindUpdated,
    Error,
};

int addControllerMappings(const std::string& mappings);
// AddMappingResult addControllerMapping(const std::string& mapping);
// std::vector<std::string> getControllerMappings();
// std::string getControllerMapping(const std::string& joystickGuid);

class Joystick {
public:
    using Ptr = std::shared_ptr<Joystick>;

    // I thought about how to make this inaccessible to user code, but it's not worth the effort.
    // Do not consider this part of the public interface, even though it is.
    static Ptr internalOpen(int devinceIndex);

    ~Joystick() { close(); }
    Joystick(const Joystick&) = delete;
    Joystick& operator=(const Joystick&) = delete;
    Joystick(Joystick&& other) = delete;
    Joystick& operator=(Joystick&& other) = delete;

    SDL_Joystick* getSdlJoystick();
    SDL_GameController* getSdlController();

    bool isOpen() const;
    void close();

    bool isConnected() const;
    // JoystickType getType() const;

    // float getAxis(int axis) const;
    // int getNumAxes() const; // negative on error
    // JoystickHatPosition getHat(int hat) const; // bitmask, see SDL_HAT_*
    // int getNumHats() const; // negative on error
    // bool getButton(int button) const;
    // int getNumButtons() const; // negative on error

    // uint16_t getVendorId() const;
    // uint16_t getProductId() const;
    // uint16_t getProductVersion() const;
    // uint16_t getFirmwareVersion() const;
    // std::string getSerialNumber() const;
    // std::string getName() const;
    // std::string getPath() const;

    // The ID (or "instance id" in SDL2 terms) is an identifier that will be unique for every
    // connected joystick. It is also changed when a joystick is reconnected.
    SDL_JoystickID getId() const;
    // The GUID is a platform-dependent identifier that is constant and unique for the type of
    // physical joystick
    SDL_JoystickGUID getGuid() const;
    std::string getGuidString() const;

    // int getPlayerIndex() const; // -1 if not available
    // void setPlayerIndex(int index) const; // -1 to clear

    // bool hasRumble() const;
    // bool rumble(uint16_t lowFrequency, uint16_t highFrequency, uint32_t durationMs) const;
    // bool hasRumbleTriggers() const;
    // bool rumbleTriggers(uint16_t left, uint16_t right, uint32_t durationMs) const;
    // bool hasLed() const;
    // bool setLed(uint8_t r, uint8_t g, uint8_t b) const;
    // JoystickPowerLevel getPowerLevel() const;

    // GameController API
    bool isController() const;
    // ControllerType getControllerType() const;
    float getAxis(ControllerAxis axis) const;
    bool getButton(ControllerButton button) const;

private:
    Joystick() = default;
    bool open(int index);

    SDL_Joystick* joystick_ = nullptr;
    SDL_GameController* controller_ = nullptr;
    SDL_JoystickID id_ = -1;
};

std::vector<Joystick::Ptr> getJoysticks();
Joystick::Ptr getJoystick(SDL_JoystickID id);

struct KeySymbol {
    Scancode scancode;
    Keycode symbol;
    KeyModifiers mod;
};

namespace events {
    struct Quit { }; // https://wiki.libsdl.org/SDL2/SDL_EventType#remarks

    struct WindowMoved {
        uint32_t windowId;
        int32_t x;
        int32_t y;
    };

    struct WindowResized {
        uint32_t windowId;
        int32_t width;
        int32_t height;
    };

    struct WindowSizeChanged {
        uint32_t windowId;
        int32_t width;
        int32_t height;
    };

    struct WindowMinimized {
        uint32_t windowId;
    };

    struct WindowMaximized {
        uint32_t windowId;
    };

    struct WindowRestored {
        uint32_t windowId;
    };

    struct WindowEnter {
        uint32_t windowId;
    };

    struct WindowLeave {
        uint32_t windowId;
    };

    struct WindowFocusGained {
        uint32_t windowId;
    };

    struct WindowFocusLost {
        uint32_t windowId;
    };

    struct WindowClose {
        uint32_t windowId;
    };

    // These are not emitted, just the joystick versions! Because I only provide a Joystick
    // abstraction (no separate abstractions).
    // struct ControllerDeviceAdded { };
    // struct ControllerDeviceRemoved { };

    struct ControllerRemapped {
        Joystick::Ptr joystick;
    };

    struct ControllerButtonDown {
        Joystick::Ptr joystick;
        ControllerButton button;
        ButtonState state;
    };

    struct ControllerButtonUp {
        Joystick::Ptr joystick;
        ControllerButton button;
        ButtonState state;
    };

    struct ControllerAxisMoved {
        Joystick::Ptr joystick;
        ControllerAxis axis;
        float value;
    };

    struct JoystickAdded {
        Joystick::Ptr joystick;
    };

    struct JoystickRemoved {
        Joystick::Ptr joystick;
    };

    struct JoystickAxisMoved {
        Joystick::Ptr joystick;
        uint8_t axis;
        float value;
    };

    struct JoystickButtonDown {
        Joystick::Ptr joystick;
        uint8_t button;
        ButtonState state;
    };

    struct JoystickButtonUp {
        Joystick::Ptr joystick;
        uint8_t button;
        ButtonState state;
    };

    struct MouseWheel {
        enum class Direction {
            Normal = SDL_MOUSEWHEEL_NORMAL,
            Flipped = SDL_MOUSEWHEEL_FLIPPED,
        };

        uint32_t windowId;
        uint32_t mouseInstanceId;
        int32_t x;
        int32_t y;
        Direction direction;
        float preciseX;
        float preciseY;
    };

    struct MouseButtonDown {
        uint32_t windowId;
        uint32_t mouseInstanceId;
        MouseButton button;
        ButtonState state;
        uint8_t clicks;
        int32_t x;
        int32_t y;
    };

    struct MouseButtonUp {
        uint32_t windowId;
        uint32_t mouseInstanceId;
        MouseButton button;
        ButtonState state;
        uint8_t clicks;
        int32_t x;
        int32_t y;
    };

    struct MouseMotion {
        uint32_t windowId;
        uint32_t mouseInstanceId;
        MouseButtonState state;
        int32_t x;
        int32_t y;
        int32_t xRel;
        int32_t yRel;
    };

    struct KeyDown {
        uint32_t windowId;
        ButtonState state;
        bool repeat;
        KeySymbol key;
    };

    struct KeyUp {
        uint32_t windowId;
        ButtonState state;
        bool repeat;
        KeySymbol key;
    };
}

using Event = std::variant<events::Quit, events::WindowMoved, events::WindowResized,
    events::WindowSizeChanged, events::WindowMinimized, events::WindowMaximized,
    events::WindowRestored, events::WindowEnter, events::WindowLeave, events::WindowFocusGained,
    events::WindowFocusLost, events::WindowClose, events::ControllerRemapped,
    events::ControllerButtonDown, events::ControllerButtonUp, events::ControllerAxisMoved,
    events::JoystickAdded, events::JoystickRemoved, events::JoystickAxisMoved,
    events::JoystickButtonDown, events::JoystickButtonUp, events::MouseWheel,
    events::MouseButtonDown, events::MouseButtonUp, events::MouseMotion, events::KeyDown,
    events::KeyUp>;

namespace detail {
    template <class... Ts>
    struct overloaded : Ts... {
        using Ts::operator()...;

        template <typename T>
        void operator()(const T&) const noexcept
        {
        }
    };
}

template <typename... Funcs>
void process(const Event& event, Funcs&&... funcs)
{
    std::visit(detail::overloaded { std::forward<Funcs>(funcs)... }, event);
}

namespace detail {
    template <typename T>
    std::vector<std::function<void(const T&)>>& getEventCallbacks()
    {
        static std::vector<std::function<void(const T&)>> callbacks;
        return callbacks;
    }

    template <typename T>
    T callEventCallbacks(T event)
    {
        for (const auto& cb : getEventCallbacks<std::decay_t<T>>()) {
            cb(event);
        }
        return event;
    }

    template <typename Type, typename Variant>
    struct IsTypeInVariant;

    template <typename Type, typename... Types>
    struct IsTypeInVariant<Type, std::variant<Types...>>
        : std::bool_constant<(... || std::is_same_v<Type, Types>)> { };
}

// I tried to make this thing deduce the event type from the function type, but I couldn't do it.
template <typename EventType>
void registerEventCallback(std::function<void(const EventType&)> callback)
{
    static_assert(detail::IsTypeInVariant<EventType, Event>::value);
    detail::getEventCallbacks<EventType>().push_back(std::move(callback));
}

std::optional<Event> pollEvent();

enum class MessageBoxType {
    Information = SDL_MESSAGEBOX_INFORMATION,
    Warning = SDL_MESSAGEBOX_WARNING,
    Error = SDL_MESSAGEBOX_ERROR,
};

bool showMessageBox(MessageBoxType type, const std::string& title, const std::string& message,
    SDL_Window* window = nullptr);

struct MessageBoxData {
    struct Button {
        enum class Flags {
            Empty = 0,
            ReturnKeyDefault = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            EscapeKeyDefault = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
        };

        int id;
        std::string text;
        Flags flags = Flags::Empty;
    };

    MessageBoxType type;
    std::string title;
    std::string message;
    std::vector<Button> buttons;
    SDL_Window* window = nullptr;
    // I don't need colors
};

std::optional<int> showMessageBox(const MessageBoxData& data);

// MISCELLANEOUS

std::string getError();

std::string getPlatform();

bool openUrl(const std::string& url);

std::string getClipboardText();
bool setClipboardText(const std::string& str);

float getTime();

enum class FullscreenType {
    Windowed = 0,
    Fullscreen = SDL_WINDOW_FULLSCREEN,
    FullscreenDesktop = SDL_WINDOW_FULLSCREEN_DESKTOP,
};

struct WindowProperties {
    FullscreenType fullscreen = FullscreenType::Windowed;
    bool hidden = false;
    bool borderless = false;
    bool resizable = false;
    bool maximized = false;
    bool minimized = false;
    bool allowHighDpi = true;
    bool openGl = false;
};

class Window {
public:
    static std::optional<Window> create(const std::string& title, uint32_t width, uint32_t height,
        const WindowProperties& props = {});

    ~Window();

    void destroy();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&& other);

    Window& operator=(Window&& other);

    struct Size {
        int x;
        int y;
    };

    Size getSize() const;

    void maximize() const;

    void setTitle(const std::string& title) const;

    SDL_Window* getSdlWindow() const;

    void* getNativeWindowHandle();
    void* getNativeDisplayHandle();

protected:
    Window() = default;

    bool init(
        const std::string& title, uint32_t width, uint32_t height, const WindowProperties& props);

    SDL_Window* window_ = nullptr;
};

struct ContextProperties {
    int contextMajor = 3;
    int contextMinor = 3;
    int redSize = 8;
    int greenSize = 8;
    int blueSize = 8;
    int alphaSize = 8;
    int depth = 24;
    bool stencil = false;
    bool srgb = false;
    int msaaSamples = 0;
};

class GlWindow : public Window {
public:
    static std::optional<GlWindow> create(const std::string& title, uint32_t width, uint32_t height,
        const WindowProperties& windowProps = {}, const ContextProperties& contextProps = {});

    ~GlWindow();

    void destroy();

    GlWindow(const GlWindow&) = delete;
    GlWindow& operator=(const GlWindow&) = delete;

    GlWindow(GlWindow&& other);

    GlWindow& operator=(GlWindow&& other);

    void swap() const;

    bool setSwapInterval(int interval) const;

    SDL_GLContext getSdlGlContext() const;

private:
    GlWindow() = default;

    bool init(const std::string& title, uint32_t width, uint32_t height,
        const WindowProperties& windowProps, const ContextProperties& contextProps);

    SDL_GLContext ctx_ = nullptr;
};

}
