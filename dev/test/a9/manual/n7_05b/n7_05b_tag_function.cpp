struct S {
    int x;
};

int S(void)
{
    return 123;
}

int main(void)
{
    return S() == 123 ? 0 : 1;
}
