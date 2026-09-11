#include "n7_cross_01c_guard_a.h"
#include "n7_cross_01c_guard_b.h"

int main(void)
{
    return g_guard.x == 13 ? 0 : 1;
}
