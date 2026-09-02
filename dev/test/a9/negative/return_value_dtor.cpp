#include <stdlib.h>

static int allocs;
static int frees;

struct Owner {
    int *p;
    Owner() { p = (int *)malloc(sizeof(int)); *p = 7; ++allocs; }
    Owner(const Owner& other) {
        p = (int *)malloc(sizeof(int));
        *p = *other.p;
        ++allocs;
    }
    ~Owner() { ++frees; free(p); }
};

Owner make_owner()
{
    Owner value;
    *value.p = 7;
    return value;
}

int main()
{
    {
        Owner result = make_owner();
        if (*result.p != 7)
            return 1;
    }
    return allocs == frees ? 0 : 2;
}
