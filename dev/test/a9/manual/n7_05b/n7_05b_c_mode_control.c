struct S {
    int x;
};

int main(void)
{
    struct S v;
    v.x = 1;
    return v.x == 1 ? 0 : 1;
}
