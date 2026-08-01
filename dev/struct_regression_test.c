/* struct_decl 変更のリグレッションテスト（純粋 C 構文） */
#include <stdio.h>
#include <string.h>

struct FP {
    int (*calc)(int, int);
    void (*noop)(void);
};

struct Bits {
    unsigned a : 3;
    unsigned b : 5;
    unsigned : 0;
    unsigned c : 1;
};

struct Flex {
    int n;
    int data[];
};

union U {
    int i;
    float f;
};

enum E { E_A = 1, E_B = 42 };

struct Anon {
    struct {
        int x, y;
    };
    int z;
};

static int add(int a, int b) { return a + b; }

int main(void)
{
    struct FP fp;
    struct Bits bits;
    union U u;
    struct Anon an;

    fp.calc = add;
    memset(&bits, 0, sizeof bits);
    bits.a = 5;
    bits.c = 1;
    u.i = 7;
    an.x = 1; an.y = 2; an.z = 3;

    printf("calc=%d\n", fp.calc(2, 3));
    printf("bits=%u,%u\n", (unsigned)bits.a, (unsigned)bits.c);
    printf("enum=%d\n", (int)E_B);
    printf("anon=%d%d%d\n", an.x, an.y, an.z);
    printf("flexoff=%d\n", (int)sizeof(struct Flex));
    printf("OK\n");
    return 0;
}
