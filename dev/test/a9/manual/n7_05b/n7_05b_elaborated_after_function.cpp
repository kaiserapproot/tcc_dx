struct S {
    int x;
};

int S(void);

int main(void)
{
    struct S value;
    value.x = 7;
    return value.x == 7 ? 0 : 1;
}
