/* 参照型のテスト: 参照引数・参照変数・オーバーロード・メソッド */
#include <stdio.h>

/* 参照引数（値が呼び出し元に反映されること） */
void twice(int& v) { v = v * 2; }
void swap2(int& a, int& b) { int t = a; a = b; b = t; }

/* オーバーロード解決に参照が参加すること。
   int と int& の両方がある場合は値渡し版を優先する（決定的に解決するため） */
int kind(int v) { return 1; }
int kind(int& r) { r = r + 100; return 2; }

/* 参照版しかない場合はオーバーロード集合でも正しく選ばれること */
int only(double d) { return 1; }
int only(int& r) { r = r + 7; return 2; }

/* 構造体の参照（コピーが起きないこと） */
struct Big {
    int a;
    int b;
};
void bump(struct Big& g) { g.a = g.a + 1; g.b = g.b + 1; }

/* ポインタへの参照 */
void retarget(int*& p, int* q) { p = q; }

class Box {
public:
    int v;
    void addTo(int& out) { out = out + v; }   /* メソッドの参照引数 */
    void setFrom(int& src) { v = src; }
};

int main(void)
{
    int x = 5;
    int y = 9;
    int a = 1, b = 2;
    struct Big g;
    int m = 11, n = 22;
    int* p = &m;
    class Box box;

    twice(x);
    printf("twice=%d\n", x);              /* 10 */

    swap2(a, b);
    printf("swap=%d,%d\n", a, b);         /* 2,1 */

    /* 参照変数: 別名として振る舞う */
    int& r = y;
    r = r + 1;
    printf("ref=%d y=%d\n", r, y);        /* 10 10 */

    twice(r);                              /* 参照を参照引数へ渡す */
    printf("ref2=%d y=%d\n", r, y);       /* 20 20 */

    g.a = 100; g.b = 200;
    bump(g);
    printf("big=%d,%d\n", g.a, g.b);      /* 101,201 */

    retarget(p, &n);
    printf("retarget=%d\n", *p);          /* 22 */

    box.v = 7;
    box.addTo(x);
    printf("method=%d\n", x);             /* 17 */
    box.setFrom(n);
    printf("setFrom=%d\n", box.v);        /* 22 */

    /* オーバーロード: int と int& なら値渡し版（副作用なし） */
    {
        int z = 3;
        int k = kind(z);
        printf("kind=%d z=%d\n", k, z);      /* 1, 3 */
    }
    /* オーバーロード: 参照版しか一致しないなら参照版（副作用 +7） */
    {
        int z = 3;
        int k = only(z);
        printf("only=%d z=%d\n", k, z);      /* 2, 10 */
    }

    printf("OK\n");
    return 0;
}
