// Control: explicit member call always worked before the fix.
class V {
public:
    float x;
    V(void) { x = 0.0f; }
    V operator*(V &o) {
        V r;
        r.x = x * o.x;
        return r;
    }
    V operator*(float s) {
        V r;
        r.x = x * s;
        return r;
    }
};

int main(void)
{
    V a;
    V b;
    a.x = 3.0f;
    b = a.operator*(2.0f);
    if (b.x != 6.0f)
        return 1;
    return 0;
}
