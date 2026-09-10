struct M {
    static int count;
    M(void) { ++count; }
};

int M::count;

struct B {
    M m;
};

B global_b[4];

int main(void)
{
    if (M::count != 4)
        return 1;
    return 0;
}
