/* 関数オーバーロードのテスト（自由関数・メソッド） */
#include <stdio.h>

/* 自由関数のオーバーロード */
int f(int x) { return x * 10; }
int f(double x) { return (int)(x * 100); }
int f(int a, int b) { return a + b; }
int f(const char* s) { return (int)s[0]; }

/* 宣言 → 定義の分離（同一シンボルに解決されること） */
int g(int a);
int g(double a);
int g(int a) { return a + 1; }
int g(double a) { return (int)a + 2; }

/* オーバーロードしない関数は素の名前のまま（C リンケージ互換） */
int plain(int x) { return x - 1; }

class Calc {
public:
    int acc;
    void set(int v) { acc = v; }
    void set(int a, int b) { acc = a * b; }
    void set(double d) { acc = (int)(d * 2); }
    int get() { return acc; }
    int viaThis(int v) { set(v); return get(); }        /* 暗黙 this のオーバーロード解決 */
    int viaThis2(int a, int b) { set(a, b); return acc; }
};

class Sub : public Calc {
public:
    int useBase() { set(3, 4); return get(); }          /* 基底のオーバーロード */
};

int main(void)
{
    class Calc c;
    class Sub s;

    printf("f(int)=%d\n", f(5));            /* 50 */
    printf("f(double)=%d\n", f(1.5));       /* 150 */
    printf("f(int,int)=%d\n", f(2, 3));     /* 5 */
    printf("f(char*)=%d\n", f("A"));        /* 65 */
    printf("g(int)=%d g(double)=%d\n", g(10), g(10.0));  /* 11, 12 */
    printf("plain=%d\n", plain(9));         /* 8 */

    c.set(7);
    printf("m1=%d\n", c.get());             /* 7 */
    c.set(3, 4);
    printf("m2=%d\n", c.get());             /* 12 */
    c.set(2.5);
    printf("m3=%d\n", c.get());             /* 5 */

    printf("via=%d\n", c.viaThis(8));       /* 8 */
    printf("via2=%d\n", c.viaThis2(5, 6));  /* 30 */
    printf("sub=%d\n", s.useBase());        /* 12 */

    printf("OK\n");
    return 0;
}
