/* メンバポインタのテスト（PLAN.md Phase 5） */
#include <stdio.h>

class Point {
public:
    int x;
    int y;
    void reset() { x = 0; y = 0; }
    void setX(int v) { x = v; }
    int sum() { return x + y; }
    int scaled(int k) { return (x + y) * k; }
};

class Derived : public Point {
public:
    int z;
    int total() { return x + y + z; }
};

/* メンバポインタを引数で受け取る */
int apply(class Point* p, int Point::* pm)
{
    return p->*pm;
}

int main(void)
{
    class Point p;
    class Point* pp = &p;
    class Derived d;

    /* --- フェーズ 1: データメンバポインタ --- */
    int Point::* px = &Point::x;
    int Point::* py = &Point::y;

    p.x = 5;
    p.y = 7;
    printf("read: %d %d\n", p.*px, p.*py);        /* 5 7 */

    p.*px = 10;                                    /* 書き込み */
    printf("write: x=%d\n", p.x);                  /* 10 */

    printf("arrow: %d\n", pp->*py);                /* 7 */
    pp->*py = 20;
    printf("arrow write: y=%d\n", p.y);            /* 20 */

    printf("param: %d\n", apply(&p, px));          /* 10 */

    /* 実行時に選ぶ */
    {
        int i;
        int Point::* sel;
        for (i = 0; i < 2; i++) {
            sel = i ? py : px;
            printf("sel[%d]=%d\n", i, p.*sel);      /* 10 then 20 */
        }
    }

    /* 継承したメンバへのポインタ */
    d.x = 1; d.y = 2; d.z = 3;
    printf("derived: %d\n", d.*px);                 /* 1 */

    /* --- フェーズ 2: メンバ関数ポインタ --- */
    {
        void (Point::* pmf)() = &Point::reset;
        int (Point::* pms)() = &Point::sum;
        int (Point::* pmk)(int) = &Point::scaled;
        void (Point::* pset)(int) = &Point::setX;

        p.x = 3; p.y = 4;
        printf("pms=%d\n", (p.*pms)());             /* 7 */
        printf("pmk=%d\n", (p.*pmk)(10));           /* 70 */

        (p.*pset)(100);
        printf("pset: x=%d\n", p.x);                /* 100 */

        (p.*pmf)();
        printf("after reset: %d %d\n", p.x, p.y);   /* 0 0 */

        /* アロー経由 */
        p.x = 8; p.y = 9;
        printf("arrow pmf=%d\n", (pp->*pms)());     /* 17 */

        /* 実行時に選ぶ */
        {
            int i;
            int (Point::* sel)();
            for (i = 0; i < 2; i++) {
                sel = pms;
                printf("selfn[%d]=%d\n", i, (p.*sel)());  /* 17 17 */
            }
        }
    }

    printf("OK\n");
    return 0;
}
