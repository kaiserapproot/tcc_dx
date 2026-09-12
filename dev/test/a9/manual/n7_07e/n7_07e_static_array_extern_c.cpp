struct M {
    static int count;

    M()
    {
        ++count;
    }
};

int M::count;

extern "C" {

void f()
{
    static M a[4];
    (void)a;
}

}

int main()
{
    f();
    if (M::count != 4)
        return 1;
    f();
    if (M::count != 4)
        return 2;
    return 0;
}
