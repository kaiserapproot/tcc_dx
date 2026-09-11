struct V {
    int x;

    V& operator=(const V& rhs)
    {
        x = rhs.x + 100;
        return *this;
    }
};

int main(void)
{
    V a, b;
    b.x = 5;
    a = b;
    return a.x == 105 ? 0 : 1;
}
