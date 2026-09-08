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
    int i;
    A a[4];

    M::count = 0;
    (void)a;
    if (M::count == 4)
        return 0;
    if (M::count == 0)
        return 2;
    return 1;
}
