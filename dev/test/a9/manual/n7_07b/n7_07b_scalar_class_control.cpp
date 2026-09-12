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
    M m;
    printf("CTOR_COUNT=%d\n", M::ctor_count);
    return (M::ctor_count == 1) ? 0 : 1;
}
