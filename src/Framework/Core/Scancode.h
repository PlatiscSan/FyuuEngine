#ifndef SCANCODE_H
#define SCANCODE_H

namespace Fyuu {

	enum Scancode {
        FYUU_SCANCODE_UNKNOWN = 0,

        /**
         *  \name Usage page 0x07
         *
         *  These values are from usage page 0x07 (USB keyboard page).
         */
         /* @{ */

        FYUU_SCANCODE_A = 4,
        FYUU_SCANCODE_B = 5,
        FYUU_SCANCODE_C = 6,
        FYUU_SCANCODE_D = 7,
        FYUU_SCANCODE_E = 8,
        FYUU_SCANCODE_F = 9,
        FYUU_SCANCODE_G = 10,
        FYUU_SCANCODE_H = 11,
        FYUU_SCANCODE_I = 12,
        FYUU_SCANCODE_J = 13,
        FYUU_SCANCODE_K = 14,
        FYUU_SCANCODE_L = 15,
        FYUU_SCANCODE_M = 16,
        FYUU_SCANCODE_N = 17,
        FYUU_SCANCODE_O = 18,
        FYUU_SCANCODE_P = 19,
        FYUU_SCANCODE_Q = 20,
        FYUU_SCANCODE_R = 21,
        FYUU_SCANCODE_S = 22,
        FYUU_SCANCODE_T = 23,
        FYUU_SCANCODE_U = 24,
        FYUU_SCANCODE_V = 25,
        FYUU_SCANCODE_W = 26,
        FYUU_SCANCODE_X = 27,
        FYUU_SCANCODE_Y = 28,
        FYUU_SCANCODE_Z = 29,

        FYUU_SCANCODE_1 = 30,
        FYUU_SCANCODE_2 = 31,
        FYUU_SCANCODE_3 = 32,
        FYUU_SCANCODE_4 = 33,
        FYUU_SCANCODE_5 = 34,
        FYUU_SCANCODE_6 = 35,
        FYUU_SCANCODE_7 = 36,
        FYUU_SCANCODE_8 = 37,
        FYUU_SCANCODE_9 = 38,
        FYUU_SCANCODE_0 = 39,

        FYUU_SCANCODE_RETURN = 40,
        FYUU_SCANCODE_ESCAPE = 41,
        FYUU_SCANCODE_BACKSPACE = 42,
        FYUU_SCANCODE_TAB = 43,
        FYUU_SCANCODE_SPACE = 44,

        FYUU_SCANCODE_MINUS = 45,
        FYUU_SCANCODE_EQUALS = 46,
        FYUU_SCANCODE_LEFTBRACKET = 47,
        FYUU_SCANCODE_RIGHTBRACKET = 48,
        FYUU_SCANCODE_BACKSLASH = 49, /**< Located at the lower left of the return
                                      *   key on ISO keyboards and at the right end
                                      *   of the QWERTY row on ANSI keyboards.
                                      *   Produces REVERSE SOLIDUS (backslash) and
                                      *   VERTICAL LINE in a US layout, REVERSE
                                      *   SOLIDUS and VERTICAL LINE in a UK Mac
                                      *   layout, NUMBER SIGN and TILDE in a UK
                                      *   Windows layout, DOLLAR SIGN and POUND SIGN
                                      *   in a Swiss German layout, NUMBER SIGN and
                                      *   APOSTROPHE in a German layout, GRAVE
                                      *   ACCENT and POUND SIGN in a French Mac
                                      *   layout, and ASTERISK and MICRO SIGN in a
                                      *   French Windows layout.
                                      */
        FYUU_SCANCODE_NONUSHASH = 50, /**< ISO USB keyboards actually use this code
                                      *   instead of 49 for the same key, but all
                                      *   OSes I've seen treat the two codes
                                      *   identically. So, as an implementor, unless
                                      *   your keyboard generates both of those
                                      *   codes and your OS treats them differently,
                                      *   you should generate FYUU_SCANCODE_BACKSLASH
                                      *   instead of this code. As a user, you
                                      *   should not rely on this code because SDL
                                      *   will never generate it with most (all?)
                                      *   keyboards.
                                      */
        FYUU_SCANCODE_SEMICOLON = 51,
        FYUU_SCANCODE_APOSTROPHE = 52,
        FYUU_SCANCODE_GRAVE = 53, /**< Located in the top left corner (on both ANSI
                                  *   and ISO keyboards). Produces GRAVE ACCENT and
                                  *   TILDE in a US Windows layout and in US and UK
                                  *   Mac layouts on ANSI keyboards, GRAVE ACCENT
                                  *   and NOT SIGN in a UK Windows layout, SECTION
                                  *   SIGN and PLUS-MINUS SIGN in US and UK Mac
                                  *   layouts on ISO keyboards, SECTION SIGN and
                                  *   DEGREE SIGN in a Swiss German layout (Mac:
                                  *   only on ISO keyboards), CIRCUMFLEX ACCENT and
                                  *   DEGREE SIGN in a German layout (Mac: only on
                                  *   ISO keyboards), SUPERSCRIPT TWO and TILDE in a
                                  *   French Windows layout, COMMERCIAL AT and
                                  *   NUMBER SIGN in a French Mac layout on ISO
                                  *   keyboards, and LESS-THAN SIGN and GREATER-THAN
                                  *   SIGN in a Swiss German, German, or French Mac
                                  *   layout on ANSI keyboards.
                                  */
        FYUU_SCANCODE_COMMA = 54,
        FYUU_SCANCODE_PERIOD = 55,
        FYUU_SCANCODE_SLASH = 56,

        FYUU_SCANCODE_CAPSLOCK = 57,

        FYUU_SCANCODE_F1 = 58,
        FYUU_SCANCODE_F2 = 59,
        FYUU_SCANCODE_F3 = 60,
        FYUU_SCANCODE_F4 = 61,
        FYUU_SCANCODE_F5 = 62,
        FYUU_SCANCODE_F6 = 63,
        FYUU_SCANCODE_F7 = 64,
        FYUU_SCANCODE_F8 = 65,
        FYUU_SCANCODE_F9 = 66,
        FYUU_SCANCODE_F10 = 67,
        FYUU_SCANCODE_F11 = 68,
        FYUU_SCANCODE_F12 = 69,

        FYUU_SCANCODE_PRINTSCREEN = 70,
        FYUU_SCANCODE_SCROLLLOCK = 71,
        FYUU_SCANCODE_PAUSE = 72,
        FYUU_SCANCODE_INSERT = 73, /**< insert on PC, help on some Mac keyboards (but
                                       does send code 73, not 117) */
        FYUU_SCANCODE_HOME = 74,
        FYUU_SCANCODE_PAGEUP = 75,
        FYUU_SCANCODE_DELETE = 76,
        FYUU_SCANCODE_END = 77,
        FYUU_SCANCODE_PAGEDOWN = 78,
        FYUU_SCANCODE_RIGHT = 79,
        FYUU_SCANCODE_LEFT = 80,
        FYUU_SCANCODE_DOWN = 81,
        FYUU_SCANCODE_UP = 82,

        FYUU_SCANCODE_NUMLOCKCLEAR = 83, /**< num lock on PC, clear on Mac keyboards
                                         */
        FYUU_SCANCODE_KP_DIVIDE = 84,
        FYUU_SCANCODE_KP_MULTIPLY = 85,
        FYUU_SCANCODE_KP_MINUS = 86,
        FYUU_SCANCODE_KP_PLUS = 87,
        FYUU_SCANCODE_KP_ENTER = 88,
        FYUU_SCANCODE_KP_1 = 89,
        FYUU_SCANCODE_KP_2 = 90,
        FYUU_SCANCODE_KP_3 = 91,
        FYUU_SCANCODE_KP_4 = 92,
        FYUU_SCANCODE_KP_5 = 93,
        FYUU_SCANCODE_KP_6 = 94,
        FYUU_SCANCODE_KP_7 = 95,
        FYUU_SCANCODE_KP_8 = 96,
        FYUU_SCANCODE_KP_9 = 97,
        FYUU_SCANCODE_KP_0 = 98,
        FYUU_SCANCODE_KP_PERIOD = 99,

        FYUU_SCANCODE_NONUSBACKSLASH = 100, /**< This is the additional key that ISO
                                            *   keyboards have over ANSI ones,
                                            *   located between left shift and Y.
                                            *   Produces GRAVE ACCENT and TILDE in a
                                            *   US or UK Mac layout, REVERSE SOLIDUS
                                            *   (backslash) and VERTICAL LINE in a
                                            *   US or UK Windows layout, and
                                            *   LESS-THAN SIGN and GREATER-THAN SIGN
                                            *   in a Swiss German, German, or French
                                            *   layout. */
        FYUU_SCANCODE_APPLICATION = 101, /**< windows contextual menu, compose */
        FYUU_SCANCODE_POWER = 102, /**< The USB document says this is a status flag,
                                   *   not a physical key - but some Mac keyboards
                                   *   do have a power key. */
        FYUU_SCANCODE_KP_EQUALS = 103,
        FYUU_SCANCODE_F13 = 104,
        FYUU_SCANCODE_F14 = 105,
        FYUU_SCANCODE_F15 = 106,
        FYUU_SCANCODE_F16 = 107,
        FYUU_SCANCODE_F17 = 108,
        FYUU_SCANCODE_F18 = 109,
        FYUU_SCANCODE_F19 = 110,
        FYUU_SCANCODE_F20 = 111,
        FYUU_SCANCODE_F21 = 112,
        FYUU_SCANCODE_F22 = 113,
        FYUU_SCANCODE_F23 = 114,
        FYUU_SCANCODE_F24 = 115,
        FYUU_SCANCODE_EXECUTE = 116,
        FYUU_SCANCODE_HELP = 117,    /**< AL Integrated Help Center */
        FYUU_SCANCODE_MENU = 118,    /**< Menu (show menu) */
        FYUU_SCANCODE_SELECT = 119,
        FYUU_SCANCODE_STOP = 120,    /**< AC Stop */
        FYUU_SCANCODE_AGAIN = 121,   /**< AC Redo/Repeat */
        FYUU_SCANCODE_UNDO = 122,    /**< AC Undo */
        FYUU_SCANCODE_CUT = 123,     /**< AC Cut */
        FYUU_SCANCODE_COPY = 124,    /**< AC Copy */
        FYUU_SCANCODE_PASTE = 125,   /**< AC Paste */
        FYUU_SCANCODE_FIND = 126,    /**< AC Find */
        FYUU_SCANCODE_MUTE = 127,
        FYUU_SCANCODE_VOLUMEUP = 128,
        FYUU_SCANCODE_VOLUMEDOWN = 129,
        /* not sure whether there's a reason to enable these */
        /*     FYUU_SCANCODE_LOCKINGCAPSLOCK = 130,  */
        /*     FYUU_SCANCODE_LOCKINGNUMLOCK = 131, */
        /*     FYUU_SCANCODE_LOCKINGSCROLLLOCK = 132, */
        FYUU_SCANCODE_KP_COMMA = 133,
        FYUU_SCANCODE_KP_EQUALSAS400 = 134,

        FYUU_SCANCODE_INTERNATIONAL1 = 135, /**< used on Asian keyboards, see
                                                footnotes in USB doc */
        FYUU_SCANCODE_INTERNATIONAL2 = 136,
        FYUU_SCANCODE_INTERNATIONAL3 = 137, /**< Yen */
        FYUU_SCANCODE_INTERNATIONAL4 = 138,
        FYUU_SCANCODE_INTERNATIONAL5 = 139,
        FYUU_SCANCODE_INTERNATIONAL6 = 140,
        FYUU_SCANCODE_INTERNATIONAL7 = 141,
        FYUU_SCANCODE_INTERNATIONAL8 = 142,
        FYUU_SCANCODE_INTERNATIONAL9 = 143,
        FYUU_SCANCODE_LANG1 = 144, /**< Hangul/English toggle */
        FYUU_SCANCODE_LANG2 = 145, /**< Hanja conversion */
        FYUU_SCANCODE_LANG3 = 146, /**< Katakana */
        FYUU_SCANCODE_LANG4 = 147, /**< Hiragana */
        FYUU_SCANCODE_LANG5 = 148, /**< Zenkaku/Hankaku */
        FYUU_SCANCODE_LANG6 = 149, /**< reserved */
        FYUU_SCANCODE_LANG7 = 150, /**< reserved */
        FYUU_SCANCODE_LANG8 = 151, /**< reserved */
        FYUU_SCANCODE_LANG9 = 152, /**< reserved */

        FYUU_SCANCODE_ALTERASE = 153,    /**< Erase-Eaze */
        FYUU_SCANCODE_SYSREQ = 154,
        FYUU_SCANCODE_CANCEL = 155,      /**< AC Cancel */
        FYUU_SCANCODE_CLEAR = 156,
        FYUU_SCANCODE_PRIOR = 157,
        FYUU_SCANCODE_RETURN2 = 158,
        FYUU_SCANCODE_SEPARATOR = 159,
        FYUU_SCANCODE_OUT = 160,
        FYUU_SCANCODE_OPER = 161,
        FYUU_SCANCODE_CLEARAGAIN = 162,
        FYUU_SCANCODE_CRSEL = 163,
        FYUU_SCANCODE_EXSEL = 164,

        FYUU_SCANCODE_KP_00 = 176,
        FYUU_SCANCODE_KP_000 = 177,
        FYUU_SCANCODE_THOUSANDSSEPARATOR = 178,
        FYUU_SCANCODE_DECIMALSEPARATOR = 179,
        FYUU_SCANCODE_CURRENCYUNIT = 180,
        FYUU_SCANCODE_CURRENCYSUBUNIT = 181,
        FYUU_SCANCODE_KP_LEFTPAREN = 182,
        FYUU_SCANCODE_KP_RIGHTPAREN = 183,
        FYUU_SCANCODE_KP_LEFTBRACE = 184,
        FYUU_SCANCODE_KP_RIGHTBRACE = 185,
        FYUU_SCANCODE_KP_TAB = 186,
        FYUU_SCANCODE_KP_BACKSPACE = 187,
        FYUU_SCANCODE_KP_A = 188,
        FYUU_SCANCODE_KP_B = 189,
        FYUU_SCANCODE_KP_C = 190,
        FYUU_SCANCODE_KP_D = 191,
        FYUU_SCANCODE_KP_E = 192,
        FYUU_SCANCODE_KP_F = 193,
        FYUU_SCANCODE_KP_XOR = 194,
        FYUU_SCANCODE_KP_POWER = 195,
        FYUU_SCANCODE_KP_PERCENT = 196,
        FYUU_SCANCODE_KP_LESS = 197,
        FYUU_SCANCODE_KP_GREATER = 198,
        FYUU_SCANCODE_KP_AMPERSAND = 199,
        FYUU_SCANCODE_KP_DBLAMPERSAND = 200,
        FYUU_SCANCODE_KP_VERTICALBAR = 201,
        FYUU_SCANCODE_KP_DBLVERTICALBAR = 202,
        FYUU_SCANCODE_KP_COLON = 203,
        FYUU_SCANCODE_KP_HASH = 204,
        FYUU_SCANCODE_KP_SPACE = 205,
        FYUU_SCANCODE_KP_AT = 206,
        FYUU_SCANCODE_KP_EXCLAM = 207,
        FYUU_SCANCODE_KP_MEMSTORE = 208,
        FYUU_SCANCODE_KP_MEMRECALL = 209,
        FYUU_SCANCODE_KP_MEMCLEAR = 210,
        FYUU_SCANCODE_KP_MEMADD = 211,
        FYUU_SCANCODE_KP_MEMSUBTRACT = 212,
        FYUU_SCANCODE_KP_MEMMULTIPLY = 213,
        FYUU_SCANCODE_KP_MEMDIVIDE = 214,
        FYUU_SCANCODE_KP_PLUSMINUS = 215,
        FYUU_SCANCODE_KP_CLEAR = 216,
        FYUU_SCANCODE_KP_CLEARENTRY = 217,
        FYUU_SCANCODE_KP_BINARY = 218,
        FYUU_SCANCODE_KP_OCTAL = 219,
        FYUU_SCANCODE_KP_DECIMAL = 220,
        FYUU_SCANCODE_KP_HEXADECIMAL = 221,

        FYUU_SCANCODE_LCTRL = 224,
        FYUU_SCANCODE_LSHIFT = 225,
        FYUU_SCANCODE_LALT = 226, /**< alt, option */
        FYUU_SCANCODE_LGUI = 227, /**< windows, command (apple), meta */
        FYUU_SCANCODE_RCTRL = 228,
        FYUU_SCANCODE_RSHIFT = 229,
        FYUU_SCANCODE_RALT = 230, /**< alt gr, option */
        FYUU_SCANCODE_RGUI = 231, /**< windows, command (apple), meta */

        FYUU_SCANCODE_MODE = 257,    /**< I'm not sure if this is really not covered
                                     *   by any of the above, but since there's a
                                     *   special FYUU_KMOD_MODE for it I'm adding it here
                                     */

                                     /* @} *//* Usage page 0x07 */

                                     /**
                                      *  \name Usage page 0x0C
                                      *
                                      *  These values are mapped from usage page 0x0C (USB consumer page).
                                      *
                                      *  There are way more keys in the spec than we can represent in the
                                      *  current scancode range, so pick the ones that commonly come up in
                                      *  real world usage.
                                      */
                                      /* @{ */

        FYUU_SCANCODE_AUDIONEXT = 258,
        FYUU_SCANCODE_AUDIOPREV = 259,
        FYUU_SCANCODE_AUDIOSTOP = 260,
        FYUU_SCANCODE_AUDIOPLAY = 261,
        FYUU_SCANCODE_AUDIOMUTE = 262,
        FYUU_SCANCODE_MEDIASELECT = 263,
        FYUU_SCANCODE_WWW = 264,             /**< AL Internet Browser */
        FYUU_SCANCODE_MAIL = 265,
        FYUU_SCANCODE_CALCULATOR = 266,      /**< AL Calculator */
        FYUU_SCANCODE_COMPUTER = 267,
        FYUU_SCANCODE_AC_SEARCH = 268,       /**< AC Search */
        FYUU_SCANCODE_AC_HOME = 269,         /**< AC Home */
        FYUU_SCANCODE_AC_BACK = 270,         /**< AC Back */
        FYUU_SCANCODE_AC_FORWARD = 271,      /**< AC Forward */
        FYUU_SCANCODE_AC_STOP = 272,         /**< AC Stop */
        FYUU_SCANCODE_AC_REFRESH = 273,      /**< AC Refresh */
        FYUU_SCANCODE_AC_BOOKMARKS = 274,    /**< AC Bookmarks */

        /* @} *//* Usage page 0x0C */

        /**
         *  \name Walther keys
         *
         *  These are values that Christian Walther added (for mac keyboard?).
         */
         /* @{ */

        FYUU_SCANCODE_BRIGHTNESSDOWN = 275,
        FYUU_SCANCODE_BRIGHTNESSUP = 276,
        FYUU_SCANCODE_DISPLAYSWITCH = 277, /**< display mirroring/dual display
                                               switch, video mode switch */
        FYUU_SCANCODE_KBDILLUMTOGGLE = 278,
        FYUU_SCANCODE_KBDILLUMDOWN = 279,
        FYUU_SCANCODE_KBDILLUMUP = 280,
        FYUU_SCANCODE_EJECT = 281,
        FYUU_SCANCODE_SLEEP = 282,           /**< SC System Sleep */

        FYUU_SCANCODE_APP1 = 283,
        FYUU_SCANCODE_APP2 = 284,

        /* @} *//* Walther keys */

        /**
         *  \name Usage page 0x0C (additional media keys)
         *
         *  These values are mapped from usage page 0x0C (USB consumer page).
         */
         /* @{ */

        FYUU_SCANCODE_AUDIOREWIND = 285,
        FYUU_SCANCODE_AUDIOFASTFORWARD = 286,

        /* @} *//* Usage page 0x0C (additional media keys) */

        /**
         *  \name Mobile keys
         *
         *  These are values that are often used on mobile phones.
         */
         /* @{ */

        FYUU_SCANCODE_SOFTLEFT = 287, /**< Usually situated below the display on phones and
                                          used as a multi-function feature key for selecting
                                          a software defined function shown on the bottom left
                                          of the display. */
        FYUU_SCANCODE_SOFTRIGHT = 288, /**< Usually situated below the display on phones and
                                           used as a multi-function feature key for selecting
                                           a software defined function shown on the bottom right
                                           of the display. */
        FYUU_SCANCODE_CALL = 289, /**< Used for accepting phone calls. */
        FYUU_SCANCODE_ENDCALL = 290, /**< Used for rejecting phone calls. */

        /* @} *//* Mobile keys */

        /* Add any other keys here. */

        FYUU_NUM_SCANCODES = 512 /**< not a key, just marks the number of scancodes
                                     for array bounds */
    };

}

#endif // !SCANCODE_H
