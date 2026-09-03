struct A {
    A(int value) { }
    ~A() { }
};

int main(void)
{
    goto n4_unsafe;
    {
        A a(1);
    n4_unsafe:
        return 0;
    }
}