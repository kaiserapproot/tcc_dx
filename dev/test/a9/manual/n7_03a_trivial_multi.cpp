struct V {
    int x;
    float y;
    void *p;
};

int main(void)
{
    V a, b;
    b.x = 1;
    b.y = 2.0f;
    b.p = (void *)0;
    a = b;
    return a.x == 1 ? 0 : 1;
}
