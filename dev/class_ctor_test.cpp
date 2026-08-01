/* コンストラクタ/デストラクタのテスト */
#include <stdio.h>

class Point {
public:
    int x;
    int y;
    Point(int a, int b) { x = a; y = b; printf("ctor(%d,%d)\n", a, b); }
    ~Point() { printf("dtor(%d,%d)\n", x, y); }
    int sum() { return x + y; }
};

class Simple {
public:
    int v;
    Simple() { v = 42; }        /* デフォルトコンストラクタのみ */
};

int make_val(void) { return 7; }   /* クラス型でない関数宣言が壊れないこと */
class Point* addr_of(class Point* p) { return p; }

int main(void)
{
    printf("-- enter main\n");
    {
        class Point p(3, 4);            /* 引数付き ctor */
        class Simple s;                  /* デフォルト ctor */
        printf("p.sum=%d s.v=%d\n", p.sum(), s.v);
        {
            class Point q(10, 20);       /* ネストスコープ */
            printf("q.sum=%d\n", q.sum());
        }                                /* ここで q の dtor */
        printf("-- after inner scope\n");
    }                                    /* ここで p の dtor（s は dtor なし） */
    printf("-- after outer scope\n");
    printf("make_val=%d\n", make_val());
    printf("OK\n");
    return 0;
}
