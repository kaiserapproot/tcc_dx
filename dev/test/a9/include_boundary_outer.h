// Middle header of the include-boundary test.  Pulls in the inner header and
// adds its own bodies, so the same TU has class initialization coming from
// depth 1 and depth 2.
#ifndef INCLUDE_BOUNDARY_OUTER_H
#define INCLUDE_BOUNDARY_OUTER_H

#include "include_boundary_inner.h"

// Local copy-init, one include deep.
static int outer_local_copy_init(void)
{
    B b = make_b();
    return b.v;
}

// Local direct-init, one include deep.
static int outer_local_direct_init(void)
{
    B a;
    B c(a);
    return c.v;
}

#endif
