/* static メンバのテスト: static 変数 / static メソッド / Class:: 構文 */
#include <stdio.h>

class Counter {
public:
    static int count;                    /* static データメンバ（宣言） */
    int id;
    static int get() { return count; }   /* static メソッド */
    static void reset() { Counter::count = 0; }
    void inc() { count = count + 1; id = count; }  /* メソッドから static 参照 */
};

int Counter::count = 100;                /* 定義（クラス外） */

class Child : public Counter {
public:
    int viaBase() { return count; }      /* 継承した static メンバ */
};

int main(void)
{
    class Counter a;
    class Counter b;
    class Child c;

    printf("initial=%d\n", Counter::count);   /* 100 */
    Counter::reset();
    printf("after reset=%d\n", Counter::get()); /* 0 */

    a.inc();          /* count=1, a.id=1 */
    b.inc();          /* count=2, b.id=2 */
    printf("a.id=%d b.id=%d shared=%d\n", a.id, b.id, Counter::count);

    Counter::count = 50;
    printf("via child=%d\n", c.viaBase());     /* 50 */
    printf("static via obj=%d\n", a.get());    /* obj 経由 static メソッド */

    /* 三項演算子の ':' が壊れていないこと */
    printf("ternary=%d\n", Counter::count > 10 ? 1 : 2);

    printf("sizeof(Counter)=%d\n", (int)sizeof(class Counter)); /* static は非包含 → 4 */
    printf("OK\n");
    return 0;
}
