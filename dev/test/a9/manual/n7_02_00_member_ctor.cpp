static int log[8];
static int n;

struct M {
    M(void) { log[n++] = 1; }
};

struct A {
    M m;
};

int main(void)
{
    n = 0;
    A a;
    (void)&a;
    if (n != 1 || log[0] != 1)
        return 1;
    return 0;
}
