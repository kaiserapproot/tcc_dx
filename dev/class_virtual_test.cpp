/* 仮想関数のテスト: 動的ディスパッチ */
#include <stdio.h>

class Shape {
public:
    int id;
    Shape(int i) { id = i; }
    virtual int area() { return 0; }
    virtual const char* name() { return "Shape"; }
    int describe() { return area() * 10; }   /* 非仮想から仮想を呼ぶ */
};

class Square : public Shape {
public:
    int side;
    Square(int s) : Shape(1) { side = s; }
    int area() { return side * side; }        /* virtual なしでも override */
    const char* name() { return "Square"; }
};

class Rect : public Shape {
public:
    int w, h;
    Rect(int a, int b) : Shape(2) { w = a; h = b; }
    virtual int area() { return w * h; }
    /* name() は override しない → Shape 版が使われる */
};

/* 3 段継承 */
class Cube : public Square {
public:
    Cube(int s) : Square(s) {}
    int area() { return side * side * 6; }
    const char* name() { return "Cube"; }
};

/* 基底ポインタ経由で多相的に呼ぶ */
int total_area(class Shape** list, int n)
{
    int i, sum = 0;
    for (i = 0; i < n; i++)
        sum = sum + list[i]->area();
    return sum;
}

int main(void)
{
    class Square sq(3);
    class Rect r(2, 5);
    class Cube cu(2);
    class Shape* list[3];
    class Shape* p;

    printf("direct: sq=%d r=%d cu=%d\n", sq.area(), r.area(), cu.area());
    printf("names: %s %s %s\n", sq.name(), r.name(), cu.name());

    /* 基底ポインタ経由 = 動的ディスパッチ */
    p = (class Shape*)&sq;
    printf("via base: area=%d name=%s id=%d\n", p->area(), p->name(), p->id);
    p = (class Shape*)&r;
    printf("via base: area=%d name=%s id=%d\n", p->area(), p->name(), p->id);
    p = (class Shape*)&cu;
    printf("via base: area=%d name=%s id=%d\n", p->area(), p->name(), p->id);

    list[0] = (class Shape*)&sq;
    list[1] = (class Shape*)&r;
    list[2] = (class Shape*)&cu;
    printf("total=%d\n", total_area(list, 3));   /* 9 + 10 + 24 = 43 */

    /* 非仮想メソッドから仮想メソッドを呼ぶと動的ディスパッチされる */
    printf("describe: %d %d\n", sq.describe(), cu.describe());  /* 90 240 */

    printf("OK\n");
    return 0;
}
