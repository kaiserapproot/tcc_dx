/* class 構文の最小回帰テスト
 * - class キーワード / アクセス指定子 / インラインメソッド本体 / メソッド宣言
 * - メソッドは sizeof に影響しないこと
 */
#include <stdio.h>

class Point {
    int x;
    int y;
public:
    void set(int a, int b) { x = a; y = b; }
    int getX() { return x; }
    int getY();
};

struct Rect {
    int w;
    int h;
    int area() { return w * h; }
};

int main(void)
{
    class Point p;
    struct Rect r;
    (void)p;
    r.w = 3;
    r.h = 4;
    printf("sizeof(Point)=%d\n", (int)sizeof(class Point));
    printf("sizeof(Rect)=%d\n", (int)sizeof(struct Rect));
    printf("rect=%dx%d\n", r.w, r.h);
    printf("OK\n");
    return 0;
}
