#include "bug33_link.h"

const int Util::factor = 3;

const char* Util::trim(const char* s)
{
    const char* p = s;
    while (*p != 0) {
        if (*p == '/')
            s = p + 1;
        ++p;
    }
    return s;
}

int Util::scale(int v)
{
    return v * Util::factor;
}
