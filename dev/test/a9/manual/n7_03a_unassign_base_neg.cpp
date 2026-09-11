struct B {
    const int x;
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
