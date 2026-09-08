#include "p0_header_defined_class.h"

static int ctor_count;
static int dtor_count;

P0Member::P0Member()
{
    ++ctor_count;
}

P0Member::~P0Member()
{
    ++dtor_count;
}

int main()
{
    {
        P0Outer x;
    }

    return ctor_count == 1 && dtor_count == 1 ? 0 : 1;
}
