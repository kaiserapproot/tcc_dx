// BUG-12: member operator+ returning a struct too big for registers
// (>8 bytes on x86_64 Win64) went through cpp_finish_member_call, which
// had the same this<->sret swap as the named-method call site.
class Big3 {
public:
    int a, b, c;
    Big3 operator+(const Big3 &o) {
        Big3 r;
        r.a = a + o.a;
        r.b = b + o.b;
        r.c = c + o.c;
        return r;
    }
};

int main()
{
    Big3 x, y, z;
    x.a = 1; x.b = 2; x.c = 3;
    y.a = 10; y.b = 20; y.c = 30;
    z = x + y;
    return z.a + z.b + z.c - 66;   /* 11+22+33 = 66 */
}
