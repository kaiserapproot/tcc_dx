struct M {
    static int count;
    M(void) { ++count; }
};

int M::count;

struct A {
    M m;
};

int main(void)
{
    M::count = 0;
    static A static_a;
    (void)&static_a;
    return M::count == 1 ? 0 : 1;
}
