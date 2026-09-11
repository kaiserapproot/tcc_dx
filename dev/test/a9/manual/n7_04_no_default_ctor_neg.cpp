struct M {
    M(int x) { (void)x; }
};
struct X {
    M m[2];
};
int main(void)
{
    X x;
    return 0;
}
