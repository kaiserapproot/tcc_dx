#include "n7_07c_f1_vec2_stub.h"

int main()
{
    vec2 a[4];
    int i;

    for (i = 0; i < 4; i++) {
        if (a[i].x != 0.0f || a[i].y != 0.0f)
            return 1 + i;
    }
    return 0;
}
