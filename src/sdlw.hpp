#pragma once

#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

/* TODO:
 * - Text Input: https://wiki.libsdl.org/SDL2/Tutorials-TextInput
 * - All the joystick & gamepad shit
 */

namespace sdlw {
namespace detail {
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

class Sdl {
public:
    static Sdl& instance()
    {
        static Sdl sdl;
        return sdl;
    }

    ~Sdl() { SDL_Quit(); }

    int getResult() const { return result_; }

private:
    Sdl() { result_ = SDL_Init(0); }
    Sdl(const Sdl&) = delete;
    Sdl& operator=(const Sdl&) = delete;

    // SDL_Init returns 0 on success and < 0 on error
    int result_ = -1;
};

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
    Misc1 = SDL_CONTROLLER_BUTTON_MISC1,
    Paddle1 = SDL_CONTROLLER_BUTTON_PADDLE1,
    Paddle2 = SDL_CONTROLLER_BUTTON_PADDLE2,
    Paddle3 = SDL_CONTROLLER_BUTTON_PADDLE3,
    Paddle4 = SDL_CONTROLLER_BUTTON_PADDLE4,
    Touchpad = SDL_CONTROLLER_BUTTON_TOUCHPAD,
};

enum class ControllerAxis {
    Invalid = SDL_CONTROLLER_AXIS_INVALID,
    LeftX = SDL_CONTROLLER_AXIS_LEFTX,
    LeftY = SDL_CONTROLLER_AXIS_LEFTY,
    RightX = SDL_CONTROLLER_AXIS_RIGHTX,
    RightY = SDL_CONTROLLER_AXIS_RIGHTY,
    TriggerLeft = SDL_CONTROLLER_AXIS_TRIGGERLEFT,
    TriggerRight = SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
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

struct MousePosition {
    int x;
    int y;
};

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

    struct ControllerDeviceAdded {
        int32_t joystickDeviceIndex;
    };

    struct ControllerDeviceRemoved {
        SDL_JoystickID instanceId;
    };

    struct ControllerDevicedRemapped {
        SDL_JoystickID instanceId;
    };

    struct ControllerButtonDown {
        SDL_JoystickID joystick;
        ControllerButton button;
        ButtonState state;
    };

    struct ControllerButtonUp {
        SDL_JoystickID joystick;
        ControllerButton button;
        ButtonState state;
    };

    struct ControllerAxisMoved {
        SDL_JoystickID joystick;
        ControllerAxis axis;
        int16_t value;
    };

    struct JoystickAdded {
        int32_t joystickDeviceIndex;
    };

    struct JoystickRemoved {
        SDL_JoystickID instanceId;
    };

    struct JoystickButtonDown {
        SDL_JoystickID instanceId;
        uint8_t button;
        ButtonState state;
    };

    struct JoystickButtonUp {
        SDL_JoystickID instanceId;
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
    events::WindowFocusLost, events::WindowClose, events::ControllerDeviceAdded,
    events::ControllerDeviceRemoved, events::ControllerDevicedRemapped,
    events::ControllerButtonDown, events::ControllerButtonUp, events::ControllerAxisMoved,
    events::JoystickAdded, events::JoystickRemoved, events::JoystickButtonDown,
    events::JoystickButtonUp, events::MouseWheel, events::MouseButtonDown, events::MouseButtonUp,
    events::MouseMotion, events::KeyDown, events::KeyUp>;

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
        : std::bool_constant<(... || std::is_same_v<Type, Types>)> {
    };
}

// I tried to make this thing deduce the event type from the function type, but I couldn't do it.
template <typename EventType>
void registerEventCallback(std::function<void(const EventType&)> callback)
{
    static_assert(detail::IsTypeInVariant<EventType, Event>::value);
    detail::getEventCallbacks<EventType>().push_back(std::move(callback));
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

enum class MessageBoxType {
    Information = SDL_MESSAGEBOX_INFORMATION,
    Warning = SDL_MESSAGEBOX_WARNING,
    Error = SDL_MESSAGEBOX_ERROR,
};

bool showMessageBox(MessageBoxType type, const std::string& title, const std::string& message,
    SDL_Window* window = nullptr)
{
    return SDL_ShowSimpleMessageBox(
               static_cast<Uint32>(type), title.c_str(), message.c_str(), window)
        == 0;
}

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

// MISCELLANEOUS

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
    if (!detail::initSubSystem(SDL_INIT_TIMER))
        return 0.0f;
    static const auto start = SDL_GetPerformanceCounter();
    const auto now = SDL_GetPerformanceCounter();
    return static_cast<float>(now - start) / SDL_GetPerformanceFrequency();
}

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
        const WindowProperties& props = {})
    {
        Window window;
        if (!window.init(title, width, height, props)) {
            return std::nullopt;
        }
        return window;
    }

    ~Window() { destroy(); }

    void destroy()
    {
        if (window_) {
            SDL_DestroyWindow(window_);
            window_ = nullptr;
        }
    }

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&& other)
        : window_(std::exchange(other.window_, nullptr))
    {
    }

    Window& operator=(Window&& other)
    {
        destroy();
        window_ = std::exchange(other.window_, nullptr);
        return *this;
    }

    struct Size {
        int x;
        int y;
    };

    Size getSize() const
    {
        int w = 0, h = 0;
        SDL_GetWindowSize(window_, &w, &h);
        return { w, h };
    }

    void maximize() const { SDL_MaximizeWindow(window_); }

    void setTitle(const std::string& title) const { SDL_SetWindowTitle(window_, title.c_str()); }

    SDL_Window* getSdlWindow() const { return window_; }

    void* getNativeWindowHandle()
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

    void* getNativeDisplayHandle()
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

protected:
    Window() = default;

    bool init(
        const std::string& title, uint32_t width, uint32_t height, const WindowProperties& props)
    {
        if (Sdl::instance().getResult() < 0) {
            return false;
        }

        if (!detail::initSubSystem(SDL_INIT_VIDEO)) {
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
        const WindowProperties& windowProps = {}, const ContextProperties& contextProps = {})
    {
        GlWindow window;
        if (!window.init(title, width, height, windowProps, contextProps)) {
            return std::nullopt;
        }
        return window;
    }

    ~GlWindow() { destroy(); }

    void destroy()
    {
        if (ctx_) {
            SDL_GL_DeleteContext(ctx_);
            ctx_ = nullptr;
        }
    }

    GlWindow(const GlWindow&) = delete;
    GlWindow& operator=(const GlWindow&) = delete;

    GlWindow(GlWindow&& other)
        : Window(std::move(other))
        , ctx_(std::exchange(other.ctx_, nullptr))
    {
    }

    GlWindow& operator=(GlWindow&& other)
    {
        Window::operator=(std::move(other));
        destroy();
        ctx_ = std::exchange(other.ctx_, nullptr);
        return *this;
    }

    void swap() const { SDL_GL_SwapWindow(window_); }

    bool setSwapInterval(int interval) const { return SDL_GL_SetSwapInterval(interval) == 0; }

    SDL_GLContext getSdlGlContext() const { return ctx_; }

private:
    GlWindow() = default;

    bool init(const std::string& title, uint32_t width, uint32_t height,
        const WindowProperties& windowProps, const ContextProperties& contextProps)
    {
        if (Sdl::instance().getResult() < 0) {
            return false;
        }

        if (!detail::initSubSystem(SDL_INIT_VIDEO)) {
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

    SDL_GLContext ctx_ = nullptr;
};

}
