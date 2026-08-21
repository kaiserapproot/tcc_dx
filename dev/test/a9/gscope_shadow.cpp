// G1 shadow test (plan-mandated): ::x must resolve to the file-scope x
// even while a local x shadows it, for both reads and writes.
int x = 1;
int main()
{
    int x = 2;
    if (::x != 1)
        return 1;
    if (x != 2)
        return 2;
    ::x = 5;
    if (x != 2)
        return 3;
    if (::x != 5)
        return 4;
    return 0;
}
