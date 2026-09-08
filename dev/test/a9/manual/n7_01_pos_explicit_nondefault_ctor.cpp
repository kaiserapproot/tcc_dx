struct A {
    int value;

    A(int x) { value = x; }
};

int main(void)
{
    A a(7);
    return a.value == 7 ? 0 : 1;
}
