// G-OP negative: unary '*' on a class with no operator* keeps failing
// like before (no silent acceptance).
struct Plain {
    int v;
};
int main()
{
    Plain p;
    p.v = 1;
    return *p;
}
