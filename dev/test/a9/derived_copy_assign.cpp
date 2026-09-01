struct B {
    int b;
    B() { b = 0; }
    B& operator=(const B& rhs)
    {
        b = rhs.b;
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
    dst = src;
    if (dst.b != 11)
        return 1;
    if (dst.d != 22)
        return 2;
    return 0;
}
