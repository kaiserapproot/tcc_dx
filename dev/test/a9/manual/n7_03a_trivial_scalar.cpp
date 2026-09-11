struct V {
    int x;
};

int main(void)
{
    V a, b;
    b.x = 7;
    a = b;
    return a.x == 7 ? 0 : 1;
}
