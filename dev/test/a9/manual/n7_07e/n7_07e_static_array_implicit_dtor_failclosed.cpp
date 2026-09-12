struct Inner {
    ~Inner() {}
};

struct Outer {
    Inner m;
};

void f()
{
    static Outer a[4];
    (void)a;
}

int main()
{
    f();
    return 0;
}
