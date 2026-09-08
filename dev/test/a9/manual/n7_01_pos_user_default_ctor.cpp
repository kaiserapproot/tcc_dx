struct A {
    static int count;

    A(void) { ++count; }
};

int A::count;

int main(void)
{
    A a;
    return A::count == 1 ? 0 : 1;
}
