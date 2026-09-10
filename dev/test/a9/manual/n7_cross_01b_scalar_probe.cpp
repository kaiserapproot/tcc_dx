static int make_i(void)
{
    return 5;
}

static int g = make_i();

int main(void)
{
    return g == 5 ? 0 : 1;
}
