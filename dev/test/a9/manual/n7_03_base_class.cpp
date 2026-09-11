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
    b.x = 5;
    b.y = 6;
    a = b;
    return a.x == 5 && a.y == 6 ? 0 : 1;
}
