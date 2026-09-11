// N7-07A diagnostic probe: print ctor_count for multidim local array.
#include <stdio.h>

struct M {
    static int ctor_count;

    M()
    {
        ++ctor_count;
    }
};

int M::ctor_count;

int main()
{
    M a[2][3];

    printf("CTOR_COUNT=%d\n", M::ctor_count);
    return M::ctor_count == 6 ? 0 : 1;
}
