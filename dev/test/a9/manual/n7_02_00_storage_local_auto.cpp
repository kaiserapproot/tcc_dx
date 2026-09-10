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
    A a;
    (void)&a;
    return M::count == 1 ? 0 : 1;
}
