#include "static_dtor_link.h"

int use_a(void)
{
    static Tracer t("a");
    return t.name[0] == 'a';
}
