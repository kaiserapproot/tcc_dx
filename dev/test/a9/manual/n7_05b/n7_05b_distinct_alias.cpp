typedef struct S {
    int x;
} S_t;

int S(void);

int main(void)
{
    S_t x;
    struct S y;
    x.x = 1;
    y.x = 2;
    return x.x + y.x == 3 ? 0 : 1;
}
