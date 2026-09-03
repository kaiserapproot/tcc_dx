struct P {
    int x;
};

int main(void)
{
    goto n4_unsafe_aggregate;
    P p = { 1 };

n4_unsafe_aggregate:
    return 0;
}
