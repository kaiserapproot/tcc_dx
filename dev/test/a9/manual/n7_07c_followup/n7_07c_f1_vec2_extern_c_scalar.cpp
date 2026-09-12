#include "n7_07c_f1_vec2_stub.h"

static int g_ok;

extern "C" {
void f(void)
{
    vec2 v;
    g_ok = (v.x == 0.0f && v.y == 0.0f);
}
}

int main()
{
    f();
    if (!g_ok)
        return 1;
    return 0;
}
