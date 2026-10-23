#include "implicit_dtor_sret.h"

int dtors;

Small make_small(long v) { Small r; r.m.a = v; return r; }
MemD make_mem(long v) { MemD r; r.a = v; r.m.a = v + 1; return r; }
Base24 make_base24(long v) { Base24 r; r.a = v; r.b = v + 1; r.c = v + 2; return r; }
