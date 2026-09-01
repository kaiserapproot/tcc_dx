#include <stdlib.h>

struct Owner {
    int *p;
    Owner() { p = (int *)malloc(sizeof(int)); *p = 7; }
    Owner(const Owner& other) {
        p = (int *)malloc(sizeof(int));
        *p = *other.p;
    }
    ~Owner() { free(p); }
};

Owner make_owner()
{
    Owner value;
    return value;
}

int main()
{
    Owner result = make_owner();
    return *result.p == 7 ? 0 : 1;
}