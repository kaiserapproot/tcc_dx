// G1: leading :: in expression position (function call and variable).
int gv = 7;
int gfn()
{
    return 35;
}
int main()
{
    return ::gfn() + ::gv - 42;
}
