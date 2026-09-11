struct S;

int S(void)
{
    return 1;
}

struct S {
    int x;
};

int main(void)
{
    struct S v;
    v.x = 1;
    return S() + v.x == 2 ? 0 : 1;
}
