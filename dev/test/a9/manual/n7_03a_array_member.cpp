struct M {
    int x;

    M& operator=(const M& rhs)
    {
        x = rhs.x;
        return *this;
    }
};

struct V {
    M m[4];
};

int main(void)
{
    V a, b;
    int i;
    for (i = 0; i < 4; i++)
        b.m[i].x = i + 10;
    a = b;
    return a.m[0].x == 10 && a.m[3].x == 13 ? 0 : 1;
}
