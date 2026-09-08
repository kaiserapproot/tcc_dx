struct A {
    A(void) {}
    A(int x) { (void)x; }
};

int main(void)
{
    A a;
    return 0;
}
