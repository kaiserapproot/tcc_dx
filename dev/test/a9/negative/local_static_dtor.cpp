struct S {
    S() {}
    ~S() {}
};

void f()
{
    static S s;
}

int main()
{
    f();
    return 0;
}
