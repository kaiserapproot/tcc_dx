// G-CAST guard: C mode is untouched - "int (x);" is still a DECLARATION
// of x (the functional-cast hook is C++-only).
int main(void)
{
    int (x);
    x = 3;
    return x - 3;
}
