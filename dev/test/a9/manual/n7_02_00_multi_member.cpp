struct M {
    static int count;
    M(void) { ++count; }
};

int M::count;

struct A {
    M a;
    M b;
    M c;
};

int main(void)
{
    M::count = 0;
    A x;
    (void)&x;
    return M::count == 3 ? 0 : 1;
}
