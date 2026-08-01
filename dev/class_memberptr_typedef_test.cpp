/* メンバポインタを typedef で扱えること */
#include <stdio.h>

class C {
public:
    int a;
    int m() { return a; }
    int add(int k) { return a + k; }
};

typedef int (C::* Fn)();          /* メンバ関数ポインタの typedef */
typedef int (C::* FnK)(int);      /* 引数ありの typedef */
typedef int C::* DataPtr;         /* データメンバポインタの typedef */

static int use(C* p, Fn f) { return (p->*f)(); }
static int useK(C* p, FnK f, int k) { return (p->*f)(k); }

int main(void)
{
    C c;
    Fn f = &C::m;
    FnK fk = &C::add;
    DataPtr d = &C::a;

    c.a = 5;
    printf("m=%d\n", use(&c, f));         /* 5 */
    printf("add=%d\n", useK(&c, fk, 7));  /* 12 */
    printf("data=%d\n", c.*d);            /* 5 */

    c.*d = 30;
    printf("after=%d\n", c.a);            /* 30 */

    if (use(&c, f) == 30 && useK(&c, fk, 2) == 32 && c.*d == 30) {
        printf("OK\n");
        return 0;
    }
    printf("NG\n");
    return 1;
}
