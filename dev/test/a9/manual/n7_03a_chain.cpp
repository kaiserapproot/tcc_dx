struct V {
    int x;
};

int main(void)
{
    V a, b, c;
    b.x = 1;
    c.x = 2;
    a = b = c;
    return a.x == 2 && b.x == 2 ? 0 : 1;
}
