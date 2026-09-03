int main(void)
{
    int x;

    x = 0;
    goto n4_unsafe_reference;
    int &ref = x;

n4_unsafe_reference:
    return x;
}
