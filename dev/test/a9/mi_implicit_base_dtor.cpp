// 自動基底 dtor: 派生 dtor の本体終了後に、各基底の dtor が宣言と逆順で
// 呼ばれること。順序はグローバルなログ配列で検証する。
int log_buf[8];
int log_n = 0;

class A {
public:
    int a;
    A() { a = 1; }
    ~A() { log_buf[log_n] = 1; log_n = log_n + 1; }
};

class B {
public:
    int b;
    B() { b = 2; }
    ~B() { log_buf[log_n] = 2; log_n = log_n + 1; }
};

class D : public A, public B {
public:
    int d;
    D() { d = 3; }
    ~D() { log_buf[log_n] = 3; log_n = log_n + 1; }
};

int main()
{
    {
        D o;
        if (o.a + o.b + o.d != 6)
            return 1;
    }                               // ここで ~D -> ~B -> ~A.
    if (log_n != 3)
        return 2;
    if (log_buf[0] != 3)            // 派生本体が先.
        return 3;
    if (log_buf[1] != 2)            // 後方宣言の基底 B が先.
        return 4;
    if (log_buf[2] != 1)            // 最後に A.
        return 5;
    return 0;
}
