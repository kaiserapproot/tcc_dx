/* BUG-52: on non-PE targets (Linux / macOS) the wide character constant
   L'a' fell through from case TOK_LCHAR into the C++ true / false case
   of unary() and became a bool with the value 0.  L"..." was fine. */
#include <stddef.h>
#include <stdio.h>

static int classify(wchar_t w)
{
    switch (w) {
    case L'a': return 1;
    case L'b': return 2;
    default: return 0;
    }
}

int main()
{
    wchar_t w = L'a';
    const wchar_t *s = L"ab";
    int bad = 0;

    if (L'a' != 97) { printf("L'a' = %d\n", (int)L'a'); bad = 1; }
    if (sizeof(L'a') != sizeof(wchar_t)) { printf("sizeof(L'a') = %d\n", (int)sizeof(L'a')); bad = 1; }
    if (L'\x41' != 65) { printf("L'\\x41' = %d\n", (int)L'\x41'); bad = 1; }
    if (L'z' - L'a' != 25) { printf("L'z' - L'a' = %d\n", (int)(L'z' - L'a')); bad = 1; }
    if (w != 97 || w != L'a') { printf("w = %d\n", (int)w); bad = 1; }
    if (s[0] != L'a' || s[1] != 98) { printf("s = %d %d\n", (int)s[0], (int)s[1]); bad = 1; }
    if (classify(s[0]) != 1 || classify(s[1]) != 2) { printf("classify failed\n"); bad = 1; }
    /* non-ASCII: wchar_t is 32-bit on Linux / macOS, so the value must survive as is */
    if (L'\u3042' != 0x3042) { printf("L'\\u3042' = %d\n", (int)L'\u3042'); bad = 1; }
    if (L'\U0001F600' != 0x1F600) { printf("L'\\U0001F600' = %d\n", (int)L'\U0001F600'); bad = 1; }
    return bad;
}
