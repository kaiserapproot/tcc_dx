#include "../n7_07c_followup/n7_07c_f1_vec2_stub.h"

extern "C" {
int f(void)
{
    vec2 a[4];
    int i;
    for (i = 0; i < 4; i++) {
        if (a[i].x != 0.0f || a[i].y != 0.0f)
            return 1;
    }
    return 0;
}
}

int main()
{
    return f();
}
