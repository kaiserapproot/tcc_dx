#include "bug33_link.h"

static int cmp(const char* a, const char* b)
{
    while (*a != 0 && *a == *b) {
        ++a;
        ++b;
    }
    return (int)(*a) - (int)(*b);
}

int main()
{
    /* static member FUNCTION defined in the other TU */
    if (cmp(Util::trim("dir/sub/file.cpp"), "file.cpp") != 0)
        return 1;
    if (Util::scale(4) != 12)
        return 2;
    /* static DATA member defined in the other TU */
    if (Util::factor != 3)
        return 3;
    return 0;
}
