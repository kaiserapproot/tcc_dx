struct M {
    int x;

    M& operator=(const M& rhs)
    {
        x = rhs.x;
        return *this;
    }
};

struct V {
    M m;
};

int main(void)
{
    V a, b;
    b.m.x = 9;
    a = b;
    return a.m.x == 9 ? 0 : 1;
}
