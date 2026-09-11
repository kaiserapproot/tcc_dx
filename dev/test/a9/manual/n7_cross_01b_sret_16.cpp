struct V {
    char pad[16];
    int mark;
};

static V make_v(void)
{
    V v;
    int i;
    for (i = 0; i < 16; i++)
        v.pad[i] = (char)(i + 1);
    v.mark = 16;
    return v;
}

static V g = make_v();

int main(void)
{
    int i;
    if (g.mark != 16) return 1;
    for (i = 0; i < 16; i++) {
        if (g.pad[i] != (char)(i + 1))
            return 2;
    }
    return 0;
}
