/* Shared by static_dtor_link_a.cpp / _b.cpp: the inline members are
   emitted in both objects and must not clash at link time. */
#include <stdio.h>

struct Tracer {
    const char *name;
    Tracer(const char *n) { name = n; printf("ctor %s\n", name); }
    ~Tracer() { printf("dtor %s\n", name); }
};

int use_a(void);
int use_b(void);
