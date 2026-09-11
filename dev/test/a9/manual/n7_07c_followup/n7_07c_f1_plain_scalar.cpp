struct M {
    int x;
    M(int value = 7)
    {
        x = value;
    }
};

int main()
{
    M m;
    if (m.x != 7)
        return 1;
    return 0;
}
