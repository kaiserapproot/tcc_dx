struct B {
    const int x;

    B() : x(0) {}
};

struct D : B {
    int y;
};

int main(void)
{
    D a, b;
    a = b;
    return 0;
}
