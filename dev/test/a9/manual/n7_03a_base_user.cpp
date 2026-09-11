struct B {
    int x;

    B& operator=(const B& rhs)
    {
        x = rhs.x;
        return *this;
    }
};

struct D : B {
    int y;
};

int main(void)
{
    D a, b;
    b.x = 3;
    b.y = 7;
    a = b;
    if (a.x != 3)
        return 1;
    if (a.y != 7)
        return 2;
    return 0;
}
