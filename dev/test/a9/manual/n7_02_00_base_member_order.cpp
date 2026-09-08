static int log[16];
static int n;

struct B {
    B(void) { log[n++] = 1; }
    ~B(void) { log[n++] = 6; }
};

struct M1 {
    M1(void) { log[n++] = 2; }
    ~M1(void) { log[n++] = 5; }
};

struct M2 {
    M2(void) { log[n++] = 3; }
    ~M2(void) { log[n++] = 4; }
};

struct D : B {
    M1 m1;
    M2 m2;
};

int main(void)
{
    static const int expected[6] = {1, 2, 3, 4, 5, 6};
    int i;

    n = 0;
    {
        D d;
        (void)&d;
    }
    for (i = 0; i < 6; ++i) {
        if (log[i] != expected[i])
            return 1;
    }
    return 0;
}
