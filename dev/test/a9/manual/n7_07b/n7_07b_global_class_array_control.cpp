#include <stdio.h>

struct M {
    static int ctor_count;

    M()
    {
        ++ctor_count;
    }
};

int M::ctor_count;
M g[4];

int main()
{
    printf("CTOR_COUNT=%d\n", M::ctor_count);
    return (M::ctor_count == 4) ? 0 : 1;
}
