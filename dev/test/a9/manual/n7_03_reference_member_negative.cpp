struct V {
    int x;
    int &r;

    V(int &v) : x(0), r(v) {}
};

int main(void)
{
    int n = 1;
    V a(n), b(n);
    a = b;
    return 0;
}
