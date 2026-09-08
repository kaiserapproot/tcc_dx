struct S {
    S() {}
    S(S &&) {}
};
int main(void)
{
    S a;
    S b(static_cast<S&&>(a));
    return 0;
}
