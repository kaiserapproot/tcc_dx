#include "static_dtor_link.h"

int use_b(void)
{
    static Tracer t("b");
    return t.name[0] == 'b';
}

int main()
{
    /* the second call must not construct again */
    if (!use_a() || !use_b() || !use_a() || !use_b())
        return 1;
    printf("main end\n");
    return 0;
}
