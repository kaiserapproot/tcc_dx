static int log[8];
static int n;

struct B {
    B(void) { log[n++] = 1; }
};

struct D : B {
};

int main(void)
{
    n = 0;
    D d;
    (void)&d;
    if (n != 1 || log[0] != 1)
        return 1;
    return 0;
}
