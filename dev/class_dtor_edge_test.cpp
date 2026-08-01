/* dtor エッジケース: 早期 return / 複数変数 / 継承クラスの ctor */
#include <stdio.h>

class Res {
public:
    int id;
    Res(int i) { id = i; printf("open(%d)\n", i); }
    ~Res() { printf("close(%d)\n", id); }
};

class Base2 {
public:
    int b;
    void setB(int v) { b = v; }
};

class Der2 : public Base2 {
public:
    int d;
    Der2(int x) { d = x; setB(x * 2); }   /* ctor から継承メソッド呼び出し */
};

int early(int flag)
{
    class Res r(1);
    if (flag) {
        class Res r2(2);
        return 100;              /* r2, r の dtor が走ること */
    }
    return 200;
}

int main(void)
{
    printf("ret=%d\n", early(1));
    printf("--\n");
    printf("ret=%d\n", early(0));
    printf("--\n");
    {
        class Res a(10), b(20);  /* 同一宣言文で複数 */
        (void)a; (void)b;
    }
    {
        class Der2 dd(5);
        printf("dd: d=%d b=%d\n", dd.d, dd.b);
    }
    printf("OK\n");
    return 0;
}
