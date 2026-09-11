struct V {
    char pad[32];
    int mark;
};

static V make_v(void)
{
    V v;
    int i;
    for (i = 0; i < 32; i++)
        v.pad[i] = (char)(i + 1);
    v.mark = 32;
    return v;
}

static V g = make_v();

int main(void)
{
    int i;
    if (g.mark != 32) return 1;
    for (i = 0; i < 32; i++) {
        if (g.pad[i] != (char)(i + 1))
            return 2;
    }
    return 0;
}
