struct POD {
    int x;
};

void f()
{
    static POD a[4];
    a[0].x = 1;
    (void)a;
}

int main()
{
    f();
    f();
    return 0;
}
