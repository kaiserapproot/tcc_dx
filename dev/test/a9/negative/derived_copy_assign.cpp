static int base_assigns;

struct B {
    int b;
    B() { b = 0; }
    B& operator=(const B& rhs)
    {
        ++base_assigns;
        b = rhs.b + 100;
        return *this;
    }
};

struct D : B {
    int d;
    D() { d = 0; }
};

int main()
{
    D src;
    D dst;

    src.b = 11;
    src.d = 22;
    dst.b = 1;
    dst.d = 2;
    base_assigns = 0;
    dst = src;
    if (base_assigns != 1)
        return 1;
    if (dst.b != 111)
        return 2;
    if (dst.d != 22)
        return 3;
    return 0;
}
