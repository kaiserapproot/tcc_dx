/* Global objects defined in a header (included by
   header_global_ctor_dtor.cpp): constructed at startup and destroyed at
   exit like those defined in the .cpp itself. */
#ifndef HEADER_GLOBAL_CTOR_DTOR_H
#define HEADER_GLOBAL_CTOR_DTOR_H

extern int g_destroyed;

struct HA { int value; ~HA() { ++g_destroyed; } };
struct HW { int v; HW() : v(7) {} ~HW() { ++g_destroyed; } };
struct HS { static HW sw; };

HA h_init = { 123 };     /* initializer list: destroyed only */
HW h_def;                /* default constructed */
HW h_args[2];            /* array: 2 */
HW HS::sw;               /* static data member definition */

#endif
