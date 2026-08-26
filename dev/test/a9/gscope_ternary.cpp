// G1: ':' handling must not break the ternary operator in C++ TUs,
// including the "cond ? a : ::b" adjacency where ':' and '::' meet.
int b = 3;
int main()
{
    int a = 1;
    int r1 = a ? 0 : 1;
    int r2 = (a == 0) ? 7 : ::b;
    int r3 = a ? ::b : 7;
    return r1 + (r2 - 3) + (r3 - 3);
}
