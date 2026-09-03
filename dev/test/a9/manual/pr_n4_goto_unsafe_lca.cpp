struct A {
    A(int value) { }
    ~A() { }
};

int main(void)
{
    {
        goto n4_unsafe_lca;
    }
    A a(60);
    {
    n4_unsafe_lca:
        return 0;
    }
}
