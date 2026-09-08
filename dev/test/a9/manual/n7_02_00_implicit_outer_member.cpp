static int marker;

struct M {
    M(void) { marker = 1; }
};

struct A {
    M m;
};

int main(void)
{
    marker = 0;
    A a;
    (void)&a;
    return marker == 1 ? 0 : 1;
}
