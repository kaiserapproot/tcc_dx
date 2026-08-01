/* Stage 4 最小テスト: 単一継承 */
#include <stdio.h>

class Base {
public:
    int x;
    void setX(int v) { x = v; }
    int getX() { return x; }
    int twice() { return getX() * 2; }
};

class Derived : public Base {
public:
    int y;
    void setY(int v) { y = v; }
    int sum() { return x + y; }          /* 基底メンバへ直接アクセス */
    int viaBase() { return getX() + y; } /* 基底メソッド呼び出し */
    int getX() { return x + 1000; }      /* メソッド隠蔽（派生優先） */
};

class Deep : public Derived {
public:
    int z;
    int total() { return x + y + z; }    /* 2 段継承 */
};

int main(void)
{
    class Base b;
    class Derived d;
    class Deep e;

    b.setX(10);
    printf("base: x=%d twice=%d\n", b.getX(), b.twice());

    d.setX(3);      /* 継承したメソッド */
    d.setY(4);
    printf("derived: x=%d y=%d sum=%d viaBase=%d\n", d.x, d.y, d.sum(), d.viaBase());
    printf("hidden: d.getX()=%d\n", d.getX());   /* 1003 (派生版) */

    e.setX(1);
    e.setY(2);
    e.z = 3;
    printf("deep: total=%d\n", e.total());

    printf("sizeof: Base=%d Derived=%d Deep=%d\n",
        (int)sizeof(class Base), (int)sizeof(class Derived), (int)sizeof(class Deep));
    printf("OK\n");
    return 0;
}
