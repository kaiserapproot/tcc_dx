static int log[16];
static int n;

struct B {
    B()  { log[n++] = 1; }
    ~B() { log[n++] = 6; }
};

struct M1 {
    M1()  { log[n++] = 2; }
    ~M1() { log[n++] = 5; }
};

struct M2 {
    M2()  { log[n++] = 3; }
    ~M2() { log[n++] = 4; }
};

struct D : B {
    M1 m1;
    M2 m2;
};

int main()
{
    {
        D d;
    }

    static const int expected[] = {1,2,3,4,5,6};

    for (int i = 0; i < 6; ++i)
        if (log[i] != expected[i])
            return 1;

    return 0;
}
