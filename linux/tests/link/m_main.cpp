#include "m_class.h"
int main()
{
    Counter local(1);
    if (g_counter.bump(2) != 42) return 1;   /* global ctor ran in the other TU */
    if (local.bump(1) != 2) return 2;
    if (twice(21) != 42) return 3;
    if (twice(1.0) != 2.5) return 4;
    return 0;
}
