/* C++ code produced by gperf version 3.0.1 */
/* Command-line: gperf --output-file=../../source/win32_keycodes.cpp ../../source/win32_keycodes.gperf  */
/* Computed positions: -k'1-2,$' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 10 "../../source/win32_keycodes.gperf"

#include <glsk/glsk.hpp>
//@cond
#define CINTERFACE
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "win32_keycodes.hpp"
#line 19 "../../source/win32_keycodes.gperf"
struct keystring_entry { const char* name; unsigned char code; };
enum
  {
    TOTAL_KEYWORDS = 145,
    MIN_WORD_LENGTH = 1,
    MAX_WORD_LENGTH = 12,
    MIN_HASH_VALUE = 1,
    MAX_HASH_VALUE = 481
  };

/* maximum key range = 481, duplicates = 0 */

#ifndef GPERF_DOWNCASE
#define GPERF_DOWNCASE 1
static unsigned char gperf_downcase[256] =
  {
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
     30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
     45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
     60,  61,  62,  63,  64,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106,
    107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
    122,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
    135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
    165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
    195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
    255
  };
#endif

#ifndef GPERF_CASE_STRCMP
#define GPERF_CASE_STRCMP 1
static int
gperf_case_strcmp (register const char *s1, register const char *s2)
{
  for (;;)
    {
      unsigned char c1 = gperf_downcase[(unsigned char)*s1++];
      unsigned char c2 = gperf_downcase[(unsigned char)*s2++];
      if (c1 != 0 && c1 == c2)
        continue;
      return (int)c1 - (int)c2;
    }
}
#endif

class glsk::internal::keycode_hash
{
private:
  static inline unsigned int hash (const char *str, unsigned int len);
public:
  static const struct keystring_entry *lookup (const char *str, unsigned int len);
};

inline unsigned int
glsk::internal::keycode_hash::hash (register const char *str, register unsigned int len)
{
  static const unsigned short asso_values[] =
    {
      482, 482, 482, 482, 482, 482, 482, 482, 482, 482,
      482, 482, 482, 482, 482, 482, 482, 482, 482, 482,
      482, 482, 482, 482, 482, 482, 482, 482, 482, 482,
      482, 482, 482, 482, 482, 482, 482, 482, 482, 240,
      482, 482,  35,  25, 482, 190,   0, 185, 170,  90,
       95, 115, 110, 105, 165, 145, 140, 135, 235, 230,
      482, 180, 482, 482, 225,   5,  80,  40,  75,  10,
       70, 175, 160, 100, 200,  65,  20, 120,  45,  60,
       15, 150,  50,   0,  30,  35, 130,  25, 155, 125,
      195, 220, 215, 210, 205, 482, 482,   5,  80,  40,
       75,  10,  70, 175, 160, 100, 200,  65,  20, 120,
       45,  60,  15, 150,  50,   0,  30,  35, 130,  25,
      155, 125, 195, 482, 482, 482, 482, 482
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[1]];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

static const struct keystring_entry wordlist[] =
  {
    {(char*)0},
#line 52 "../../source/win32_keycodes.gperf"
    {"s", DIK_S},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 51 "../../source/win32_keycodes.gperf"
    {"a", DIK_A},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 39 "../../source/win32_keycodes.gperf"
    {"e", DIK_E},
    {(char*)0}, {(char*)0},
#line 154 "../../source/win32_keycodes.gperf"
    {"Apps", DIK_APPS},
    {(char*)0},
#line 22 "../../source/win32_keycodes.gperf"
    {"Escape", DIK_ESCAPE},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 78 "../../source/win32_keycodes.gperf"
    {"Space", DIK_SPACE},
#line 46 "../../source/win32_keycodes.gperf"
    {"p", DIK_P},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 141 "../../source/win32_keycodes.gperf"
    {"Pause", DIK_PAUSE},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 156 "../../source/win32_keycodes.gperf"
    {"Sleep", DIK_SLEEP},
#line 59 "../../source/win32_keycodes.gperf"
    {"l", DIK_L},
    {(char*)0}, {(char*)0},
#line 157 "../../source/win32_keycodes.gperf"
    {"Wake", DIK_WAKE},
    {(char*)0}, {(char*)0},
#line 159 "../../source/win32_keycodes.gperf"
    {"WebFavorites", DIK_WEBFAVORITES},
    {(char*)0},
#line 124 "../../source/win32_keycodes.gperf"
    {"Stop", DIK_STOP},
    {(char*)0},
#line 38 "../../source/win32_keycodes.gperf"
    {"w", DIK_W},
#line 136 "../../source/win32_keycodes.gperf"
    {"WebHome", DIK_WEBHOME},
    {(char*)0},
#line 132 "../../source/win32_keycodes.gperf"
    {"PlayPause", DIK_PLAYPAUSE},
    {(char*)0},
#line 63 "../../source/win32_keycodes.gperf"
    {"LShift", DIK_LSHIFT},
#line 161 "../../source/win32_keycodes.gperf"
    {"WebStop", DIK_WEBSTOP},
    {(char*)0},
#line 77 "../../source/win32_keycodes.gperf"
    {"LAlt", DIK_LMENU},
    {(char*)0},
#line 41 "../../source/win32_keycodes.gperf"
    {"t", DIK_T},
    {(char*)0}, {(char*)0},
#line 145 "../../source/win32_keycodes.gperf"
    {"Left", DIK_LEFT},
    {(char*)0},
#line 91 "../../source/win32_keycodes.gperf"
    {"Scroll", DIK_SCROLL},
#line 143 "../../source/win32_keycodes.gperf"
    {"Up", DIK_UP},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 43 "../../source/win32_keycodes.gperf"
    {"u", DIK_U},
#line 79 "../../source/win32_keycodes.gperf"
    {"Capital", DIK_CAPITAL},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 111 "../../source/win32_keycodes.gperf"
    {"Kana", DIK_KANA},
    {(char*)0},
#line 67 "../../source/win32_keycodes.gperf"
    {"c", DIK_C},
    {(char*)0}, {(char*)0},
#line 104 "../../source/win32_keycodes.gperf"
    {"Num.", DIK_DECIMAL},
#line 50 "../../source/win32_keycodes.gperf"
    {"LCtrl", DIK_LCONTROL},
#line 75 "../../source/win32_keycodes.gperf"
    {"RShift", DIK_RSHIFT},
    {(char*)0}, {(char*)0},
#line 140 "../../source/win32_keycodes.gperf"
    {"RAlt", DIK_RMENU},
    {(char*)0},
#line 70 "../../source/win32_keycodes.gperf"
    {"n", DIK_N},
    {(char*)0}, {(char*)0},
#line 152 "../../source/win32_keycodes.gperf"
    {"LWin", DIK_LWIN},
    {(char*)0},
#line 137 "../../source/win32_keycodes.gperf"
    {"NumpadComma", DIK_NUMPADCOMMA},
    {(char*)0}, {(char*)0},
#line 122 "../../source/win32_keycodes.gperf"
    {"UnderLine", DIK_UNDERLINE},
    {(char*)0},
#line 40 "../../source/win32_keycodes.gperf"
    {"r", DIK_R},
    {(char*)0}, {(char*)0},
#line 35 "../../source/win32_keycodes.gperf"
    {"Backspace", DIK_BACK},
#line 131 "../../source/win32_keycodes.gperf"
    {"Calculator", DIK_CALCULATOR},
#line 73 "../../source/win32_keycodes.gperf"
    {"Period", DIK_PERIOD},
#line 163 "../../source/win32_keycodes.gperf"
    {"WebBack", DIK_WEBBACK},
#line 151 "../../source/win32_keycodes.gperf"
    {"Del", DIK_DELETE},
#line 99 "../../source/win32_keycodes.gperf"
    {"Num+", DIK_ADD},
#line 72 "../../source/win32_keycodes.gperf"
    {"Comma", DIK_COMMA},
#line 49 "../../source/win32_keycodes.gperf"
    {"Return", DIK_RETURN},
    {(char*)0}, {(char*)0},
#line 129 "../../source/win32_keycodes.gperf"
    {"RCtl", DIK_RCONTROL},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 36 "../../source/win32_keycodes.gperf"
    {"Tab", DIK_TAB},
#line 76 "../../source/win32_keycodes.gperf"
    {"Num*", DIK_MULTIPLY},
#line 162 "../../source/win32_keycodes.gperf"
    {"WebForward", DIK_WEBFORWARD},
#line 45 "../../source/win32_keycodes.gperf"
    {"o", DIK_O},
    {(char*)0}, {(char*)0},
#line 153 "../../source/win32_keycodes.gperf"
    {"RWin", DIK_RWIN},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 127 "../../source/win32_keycodes.gperf"
    {"NextTrack", DIK_NEXTTRACK},
#line 155 "../../source/win32_keycodes.gperf"
    {"Power", DIK_POWER},
#line 58 "../../source/win32_keycodes.gperf"
    {"k", DIK_K},
    {(char*)0},
#line 147 "../../source/win32_keycodes.gperf"
    {"End", DIK_END},
#line 128 "../../source/win32_keycodes.gperf"
    {"NumReturn", DIK_NUMPADENTER},
    {(char*)0}, {(char*)0},
#line 113 "../../source/win32_keycodes.gperf"
    {"Convert", DIK_CONVERT},
    {(char*)0},
#line 118 "../../source/win32_keycodes.gperf"
    {"PrevTrack", DIK_PREVTRACK},
    {(char*)0},
#line 54 "../../source/win32_keycodes.gperf"
    {"f", DIK_F},
    {(char*)0}, {(char*)0},
#line 114 "../../source/win32_keycodes.gperf"
    {"NoConvert", DIK_NOCONVERT},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 150 "../../source/win32_keycodes.gperf"
    {"Ins", DIK_INSERT},
#line 165 "../../source/win32_keycodes.gperf"
    {"Mail", DIK_MAIL},
    {(char*)0},
#line 53 "../../source/win32_keycodes.gperf"
    {"d", DIK_D},
#line 90 "../../source/win32_keycodes.gperf"
    {"NumLock", DIK_NUMLOCK},
    {(char*)0},
#line 133 "../../source/win32_keycodes.gperf"
    {"MediaStop", DIK_MEDIASTOP},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 69 "../../source/win32_keycodes.gperf"
    {"b", DIK_B},
    {(char*)0}, {(char*)0},
#line 126 "../../source/win32_keycodes.gperf"
    {"UnLabeled", DIK_UNLABELED},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 130 "../../source/win32_keycodes.gperf"
    {"Mute", DIK_MUTE},
    {(char*)0},
#line 166 "../../source/win32_keycodes.gperf"
    {"MediaSelect", DIK_MEDIASELECT},
    {(char*)0}, {(char*)0},
#line 100 "../../source/win32_keycodes.gperf"
    {"Num1", DIK_NUMPAD1},
#line 123 "../../source/win32_keycodes.gperf"
    {"Kanji", DIK_KANJI},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 101 "../../source/win32_keycodes.gperf"
    {"Num2", DIK_NUMPAD2},
    {(char*)0},
#line 23 "../../source/win32_keycodes.gperf"
    {"1", DIK_1},
#line 112 "../../source/win32_keycodes.gperf"
    {"Abnt_C1", DIK_ABNT_C1},
#line 115 "../../source/win32_keycodes.gperf"
    {"Yen", DIK_YEN},
#line 148 "../../source/win32_keycodes.gperf"
    {"Down", DIK_DOWN},
#line 146 "../../source/win32_keycodes.gperf"
    {"Right", DIK_RIGHT},
    {(char*)0},
#line 116 "../../source/win32_keycodes.gperf"
    {"Abnt_C2", DIK_ABNT_C2},
    {(char*)0},
#line 97 "../../source/win32_keycodes.gperf"
    {"Num5", DIK_NUMPAD5},
    {(char*)0},
#line 24 "../../source/win32_keycodes.gperf"
    {"2", DIK_2},
    {(char*)0},
#line 105 "../../source/win32_keycodes.gperf"
    {"OEM", DIK_OEM_102},
#line 96 "../../source/win32_keycodes.gperf"
    {"Num4", DIK_NUMPAD4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 102 "../../source/win32_keycodes.gperf"
    {"Num3", DIK_NUMPAD3},
    {(char*)0},
#line 44 "../../source/win32_keycodes.gperf"
    {"i", DIK_I},
    {(char*)0}, {(char*)0},
#line 158 "../../source/win32_keycodes.gperf"
    {"WebSearch", DIK_WEBSEARCH},
#line 160 "../../source/win32_keycodes.gperf"
    {"WebRefresh", DIK_WEBREFRESH},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 144 "../../source/win32_keycodes.gperf"
    {"PgUp", DIK_PRIOR},
    {(char*)0},
#line 27 "../../source/win32_keycodes.gperf"
    {"5", DIK_5},
    {(char*)0},
#line 135 "../../source/win32_keycodes.gperf"
    {"VolumeUp", DIK_VOLUMEUP},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 94 "../../source/win32_keycodes.gperf"
    {"Num9", DIK_NUMPAD9},
    {(char*)0},
#line 26 "../../source/win32_keycodes.gperf"
    {"4", DIK_4},
    {(char*)0}, {(char*)0},
#line 93 "../../source/win32_keycodes.gperf"
    {"Num8", DIK_NUMPAD8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 92 "../../source/win32_keycodes.gperf"
    {"Num7", DIK_NUMPAD7},
    {(char*)0},
#line 25 "../../source/win32_keycodes.gperf"
    {"3", DIK_3},
    {(char*)0}, {(char*)0},
#line 142 "../../source/win32_keycodes.gperf"
    {"Home", DIK_HOME},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 149 "../../source/win32_keycodes.gperf"
    {"PgDn", DIK_NEXT},
#line 62 "../../source/win32_keycodes.gperf"
    {"Grave", DIK_GRAVE},
#line 71 "../../source/win32_keycodes.gperf"
    {"m", DIK_M},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 134 "../../source/win32_keycodes.gperf"
    {"VolumeDown", DIK_VOLUMEDOWN},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 98 "../../source/win32_keycodes.gperf"
    {"Num6", DIK_NUMPAD6},
    {(char*)0},
#line 42 "../../source/win32_keycodes.gperf"
    {"y", DIK_Y},
#line 80 "../../source/win32_keycodes.gperf"
    {"F1", DIK_F1},
#line 106 "../../source/win32_keycodes.gperf"
    {"F11", DIK_F11},
#line 103 "../../source/win32_keycodes.gperf"
    {"Num0", DIK_NUMPAD0},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 107 "../../source/win32_keycodes.gperf"
    {"F12", DIK_F12},
    {(char*)0}, {(char*)0},
#line 68 "../../source/win32_keycodes.gperf"
    {"v", DIK_V},
#line 81 "../../source/win32_keycodes.gperf"
    {"F2", DIK_F2},
    {(char*)0},
#line 117 "../../source/win32_keycodes.gperf"
    {"Num=", DIK_NUMPADEQUALS},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 110 "../../source/win32_keycodes.gperf"
    {"F15", DIK_F15},
#line 138 "../../source/win32_keycodes.gperf"
    {"Num/", DIK_DIVIDE},
    {(char*)0},
#line 31 "../../source/win32_keycodes.gperf"
    {"9", DIK_9},
    {(char*)0},
#line 109 "../../source/win32_keycodes.gperf"
    {"F14", DIK_F14},
#line 95 "../../source/win32_keycodes.gperf"
    {"Num-", DIK_SUBTRACT},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 108 "../../source/win32_keycodes.gperf"
    {"F13", DIK_F13},
    {(char*)0},
#line 139 "../../source/win32_keycodes.gperf"
    {"SysRq", DIK_SYSRQ},
#line 30 "../../source/win32_keycodes.gperf"
    {"8", DIK_8},
#line 84 "../../source/win32_keycodes.gperf"
    {"F5", DIK_F5},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 29 "../../source/win32_keycodes.gperf"
    {"7", DIK_7},
#line 83 "../../source/win32_keycodes.gperf"
    {"F4", DIK_F4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 37 "../../source/win32_keycodes.gperf"
    {"q", DIK_Q},
#line 82 "../../source/win32_keycodes.gperf"
    {"F3", DIK_F3},
    {(char*)0}, {(char*)0},
#line 164 "../../source/win32_keycodes.gperf"
    {"MyComputer", DIK_MYCOMPUTER},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 66 "../../source/win32_keycodes.gperf"
    {"x", DIK_X},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 125 "../../source/win32_keycodes.gperf"
    {"AX", DIK_AX},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 56 "../../source/win32_keycodes.gperf"
    {"h", DIK_H},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 28 "../../source/win32_keycodes.gperf"
    {"6", DIK_6},
    {(char*)0},
#line 89 "../../source/win32_keycodes.gperf"
    {"F10", DIK_F10},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 32 "../../source/win32_keycodes.gperf"
    {"0", DIK_0},
#line 88 "../../source/win32_keycodes.gperf"
    {"F9", DIK_F9},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 55 "../../source/win32_keycodes.gperf"
    {"g", DIK_G},
#line 87 "../../source/win32_keycodes.gperf"
    {"F8", DIK_F8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 34 "../../source/win32_keycodes.gperf"
    {"=", DIK_EQUALS},
#line 86 "../../source/win32_keycodes.gperf"
    {"F7", DIK_F7},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 74 "../../source/win32_keycodes.gperf"
    {"/", DIK_SLASH},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 33 "../../source/win32_keycodes.gperf"
    {"-", DIK_MINUS},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 65 "../../source/win32_keycodes.gperf"
    {"z", DIK_Z},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 57 "../../source/win32_keycodes.gperf"
    {"j", DIK_J},
#line 85 "../../source/win32_keycodes.gperf"
    {"F6", DIK_F6},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 119 "../../source/win32_keycodes.gperf"
    {"^", DIK_CIRCUMFLEX},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 48 "../../source/win32_keycodes.gperf"
    {"]", DIK_RBRACKET},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 64 "../../source/win32_keycodes.gperf"
    {"\\", DIK_BACKSLASH},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 47 "../../source/win32_keycodes.gperf"
    {"[", DIK_LBRACKET},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 120 "../../source/win32_keycodes.gperf"
    {"@", DIK_AT},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 60 "../../source/win32_keycodes.gperf"
    {";", DIK_SEMICOLON},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 121 "../../source/win32_keycodes.gperf"
    {":", DIK_COLON},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 61 "../../source/win32_keycodes.gperf"
    {"'", DIK_APOSTROPHE}
  };

const struct keystring_entry *
glsk::internal::keycode_hash::lookup (register const char *str, register unsigned int len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].name;

          if (s && (((unsigned char)*str ^ (unsigned char)*s) & ~32) == 0 && !gperf_case_strcmp (str, s))
            return &wordlist[key];
        }
    }
  return 0;
}
#line 167 "../../source/win32_keycodes.gperf"



//@endcond

/** Get the keycode by the name of a key.
	The algorithm employed needs a single string match to find the right keycode,
	however it is good practice to only get the keycodes once and not everytime when
	checking for a keystate.
	\param name The name of the key as a human readable string.
	\return Corresponding system dependant keycode.
	\ingroup Input
*/
int glsk::get_keycode( const std::string& name )
{
	const struct keystring_entry* result = 0;

	result = glsk::internal::keycode_hash::lookup( name.c_str(), (unsigned int)name.length() );

	return (result) ? result->code : 0;
}

/** Get the name of a key by a keycode.
	\param code Keycode of the key.
	\return The name of the corresponing key as a string.
	\ingroup Input
*/
const char* glsk::get_keyname( int code )
{
	int n = sizeof( wordlist ) / sizeof( struct keystring_entry );
	int i;

	for ( i = 0; i < n; ++i )
	{
		if ( wordlist[ i ].name && ( wordlist[ i ].code == code ) )
			return wordlist[ i ].name;
	}

	return "";
}
