struct M {
    static int count;
    M(void) { ++count; }
};

int M::count;

struct A {
    M m;
};

A global_a;

int main(void)
{
    if (M::count != 1)
        return 1;
    return 0;
}
