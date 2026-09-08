struct M {
    static int count;
    M(void) { ++count; }
};

int M::count;

struct A {
    M m;
};

A global_a[4];

int main(void)
{
    M::count = 0;
    if (M::count != 4)
        return 1;
    return 0;
}
