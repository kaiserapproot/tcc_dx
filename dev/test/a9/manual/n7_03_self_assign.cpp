struct M {
    int x;

    M& operator=(const M& rhs)
    {
        x = rhs.x + 1;
        return *this;
    }
};

struct V {
    M m;
    int y;
};

int main(void)
{
    V a;
    a.m.x = 5;
    a.y = 7;
    a = a;
    return a.m.x == 6 && a.y == 7 ? 0 : 1;
}
