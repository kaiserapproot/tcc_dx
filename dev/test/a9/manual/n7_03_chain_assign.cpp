struct V {
    int x;
    int y;
};

int main(void)
{
    V a, b, c;
    c.x = 1;
    c.y = 2;
    b.x = 0;
    b.y = 0;
    a.x = 0;
    a.y = 0;
    a = b = c;
    return a.x == 1 && a.y == 2 && b.x == 1 && b.y == 2 ? 0 : 1;
}
