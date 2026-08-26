// G1 negative: "::y" with no file-scope y must be rejected (C++ has no
// implicit declaration; falling back to the local would be a silent
// miscompile).
int main()
{
    int y = 2;
    return ::y;
}
