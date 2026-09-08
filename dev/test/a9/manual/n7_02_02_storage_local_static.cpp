static int slog[8];
static int sn;

struct B {
    B()  { slog[sn++] = 1; }
    ~B() { slog[sn++] = 4; }
};

struct M {
    M()  { slog[sn++] = 2; }
    ~M() { slog[sn++] = 3; }
};

struct D : B {
    M m;
};

int f()
{
    static D s;
    if (sn != 2 || slog[0] != 1 || slog[1] != 2)
        return 1;
    return 0;
}

int main()
{
    if (f() != 0)
        return 1;
    if (f() != 0)
        return 1;
    return 0;
}
