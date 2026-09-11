struct V {
    char pad[128];
    int mark;
};

static V make_v(void)
{
    V v;
    int i;
    for (i = 0; i < 128; i++)
        v.pad[i] = (char)(i + 1);
    v.mark = 128;
    return v;
}

static V g = make_v();

int main(void)
{
    int i;
    if (g.mark != 128) return 1;
    for (i = 0; i < 128; i++) {
        if (g.pad[i] != (char)(i + 1))
            return 2;
    }
    return 0;
}
