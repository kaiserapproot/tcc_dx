struct V {
    int &x;
};

int main(void)
{
    int n = 0;
    V a = { n };
    V b = { n };
    a = b;
    return 0;
}
