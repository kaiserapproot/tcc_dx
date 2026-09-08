struct A {
    A(int x) { (void)x; }
};

int main(void)
{
    static A static_a;
    return 0;
}
