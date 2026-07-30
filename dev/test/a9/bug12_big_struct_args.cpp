// BUG-12 regression: big-struct-return method WITH a user argument, to
// verify the arg after `this` still lands correctly (R8) once sret/this
// occupy RCX/RDX.
class Big3 {
public:
    int a, b, c;
    Big3 scaled(int k);
};

Big3 Big3::scaled(int k)
{
    Big3 r;
    r.a = a * k;
    r.b = b * k;
    r.c = c * k;
    return r;
}

int main()
{
    Big3 x, y;
    x.a = 1; x.b = 2; x.c = 3;
    y = x.scaled(10);
    return y.a + y.b + y.c - 60;   /* 10+20+30 */
}
