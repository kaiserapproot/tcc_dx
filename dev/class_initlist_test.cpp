/* 初期化子リストと基底 ctor/dtor 連鎖のテスト */
#include <stdio.h>

class Base {
public:
    int bv;
    Base(int v) { bv = v; printf("Base ctor(%d)\n", v); }
    ~Base() { printf("Base dtor(%d)\n", bv); }
};

class Derived : public Base {
public:
    int dv;
    Derived(int b, int d) : Base(b), dv(d) { printf("Derived ctor(%d,%d)\n", b, d); }
    ~Derived() { printf("Derived dtor(%d)\n", dv); }
    int sum() { return bv + dv; }
};

class DefBase {
public:
    int n;
    DefBase() { n = 7; printf("DefBase ctor\n"); }
    ~DefBase() { printf("DefBase dtor\n"); }
};

class DefChild : public DefBase {
public:
    int m;
    DefChild(int x) { m = x; printf("DefChild ctor(%d)\n", x); }
    /* dtor なし → 基底 dtor が直接呼ばれること */
};

int main(void)
{
    {
        class Derived d(10, 20);   /* Base(10) → dv=20 → Derived 本体 */
        printf("sum=%d\n", d.sum());
    }                              /* Derived dtor → Base dtor */
    printf("--\n");
    {
        class DefChild c(5);       /* DefBase() 暗黙呼び出し → m=5 */
        printf("c: n=%d m=%d\n", c.n, c.m);
    }                              /* DefBase dtor（DefChild は dtor なし） */
    printf("OK\n");
    return 0;
}
