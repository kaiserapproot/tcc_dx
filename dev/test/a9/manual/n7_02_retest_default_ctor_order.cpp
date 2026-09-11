// Amateras supplementary: default ctor after other ctors must still work.
class V {
public:
    float x;
    V(V &o) { x = o.x; }
    V(float f) { x = f; }
    V(void) { x = 0.0f; }
};

int main(void)
{
    V a;
    if (a.x != 0.0f)
        return 1;
    return 0;
}
