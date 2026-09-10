struct V {
    int x;
};

static V v = { 3 };

int main(void)
{
    return v.x == 3 ? 0 : 1;
}
