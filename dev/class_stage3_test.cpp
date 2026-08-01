/* Stage 3 最小テスト: メソッド呼び出し・this 経由メンバアクセス */
#include <stdio.h>

class Point {
    int x;
    int y;
public:
    void set(int a, int b) { x = a; y = b; }
    int getX() { return x; }
    int getY() { return this->y; }
    int sum() { return getX() + getY(); }   /* メソッドからメソッド呼び出し */
};

struct Rect {
    int w;
    int h;
    int area() { return w * h; }
};

class Counter {
    int n;
public:
    void set(int v) { n = v; }      /* Point::set と同名（マングリング検証） */
    int get() { return n; }
    void add(int d) { n = n + d; }
};

int main(void)
{
    class Point p;
    struct Rect r;
    class Counter c;
    class Point* pp = &p;

    p.set(3, 4);
    printf("p=(%d,%d) sum=%d\n", p.getX(), p.getY(), p.sum());

    r.w = 5;
    r.h = 6;
    printf("area=%d\n", r.area());

    c.set(100);
    c.add(23);
    printf("counter=%d\n", c.get());

    pp->set(7, 8);                   /* アロー経由 */
    printf("pp=(%d,%d)\n", pp->getX(), pp->getY());

    printf("sizeof(Point)=%d\n", (int)sizeof(class Point));
    printf("OK\n");
    return 0;
}
