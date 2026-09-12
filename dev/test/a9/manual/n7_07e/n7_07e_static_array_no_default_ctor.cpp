struct M {
    M(int);
};

void f()
{
    static M a[4];
    (void)a;
}

int main()
{
    f();
    return 0;
}
