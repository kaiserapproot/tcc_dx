// Amateras authority repro (vec_quat_matrix_cpp shape):
// operator*(V&) declared before operator*(float); `a * 2.0f` must pick float.
class V {
public:
    float x;
    V(void) { x = 0.0f; }
    V(float f) { x = f; }
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
    b = a * 2.0f;
    if (b.x != 6.0f)
        return 1;
    return 0;
}
