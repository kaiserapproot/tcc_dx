// G1 guard: C-mode ':' paths (ternary, label, bitfield) stay untouched
// by the C++ leading-:: support (compile gate; C has no '::').
struct bf {
    int a : 3;
};
int main()
{
    int a = 1;
    int r = a ? 0 : 1;
    struct bf b;
    b.a = 2;
    if (a > 0)
        goto done;
done:
    return r + (b.a - 2);
}
