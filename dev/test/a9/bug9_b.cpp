// BUG-9: local reference to a scalar
int main()
{
    int x;
    x = 7;
    int &r = x;
    r += 1;
    return x - 8;
}
