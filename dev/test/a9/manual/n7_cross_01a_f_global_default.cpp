struct V {
    int x;
    V(void) { x = 7; }
};

static V g;

int main(void)
{
    return g.x == 7 ? 0 : 1;
}
