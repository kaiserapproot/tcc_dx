/* 純粋仮想関数・抽象クラス・new/delete・仮想デストラクタ
   （クラス名をそのまま型名として使う書き方も検証する） */
#include <stdio.h>

/* 抽象基底クラス（インタフェース） */
class Shape {
public:
    int id;
    Shape(int i) { id = i; }
    virtual ~Shape() { printf("~Shape(%d)\n", id); }
    virtual int area() = 0;               /* 純粋仮想 */
    virtual const char* name() = 0;
    int twice() { return area() * 2; }
};

class Square : public Shape {
public:
    int side;
    Square(int s) : Shape(1) { side = s; printf("Square(%d)\n", s); }
    ~Square() { printf("~Square(%d)\n", side); }
    int area() { return side * side; }
    const char* name() { return "Square"; }
};

class Rect : public Shape {
public:
    int w, h;
    Rect(int a, int b) : Shape(2) { w = a; h = b; }
    int area() { return w * h; }          /* ~Rect は無いので基底の dtor が使われる */
    const char* name() { return "Rect"; }
};

/* ヒープ上のオブジェクトを基底ポインタで扱う */
int sum_and_free(Shape** list, int n)
{
    int i, sum = 0;
    for (i = 0; i < n; i++) {
        sum = sum + list[i]->area();
        delete list[i];                    /* 仮想デストラクタ経由 */
    }
    return sum;
}

int main(void)
{
    Shape* list[2];
    Square local(6);                       /* クラス名を型名として使う */
    Square* sp;

    printf("local: %s area=%d twice=%d\n", local.name(), local.area(), local.twice());

    sp = new Square(4);                    /* new + ctor */
    printf("heap: %s area=%d\n", sp->name(), sp->area());
    delete sp;                             /* dtor -> ~Square, ~Shape */

    list[0] = (Shape*)new Square(3);
    list[1] = (Shape*)new Rect(2, 5);
    printf("sum=%d\n", sum_and_free(list, 2));   /* 9 + 10 = 19 */

    printf("OK\n");
    return 0;
}
