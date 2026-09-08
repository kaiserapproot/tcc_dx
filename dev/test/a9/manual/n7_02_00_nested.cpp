static int marker;

struct M {
    M(void) { marker = 1; }
};

struct A {
    M m;
};

struct B {
    A a;
};

B global_b;

int main(void)
{
    marker = 0;
    if (marker != 1)
        return 1;
    return 0;
}
