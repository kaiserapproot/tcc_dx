struct M {
    int x;
};

struct V {
    M m;
    int y;
};

int main(void)
{
    V a, b;
    b.m.x = 3;
    b.y = 4;
    a = b;
    return a.m.x == 3 && a.y == 4 ? 0 : 1;
}
