// N6-06B-00: global ctor/dtor visibility for direct relocate manual call.
#include <stdio.h>
#include <string.h>

static volatile int g_global_ctor_done;
static volatile int g_global_dtor_done;

static void probe_log(const char *msg)
{
    if (strcmp(msg, "GLOBAL_CTOR") == 0)
        g_global_ctor_done = 1;
    if (strcmp(msg, "GLOBAL_DTOR") == 0)
        g_global_dtor_done = 1;
    printf("%s\n", msg);
    fflush(stdout);
}

struct GlobalObj {
    GlobalObj() { probe_log("GLOBAL_CTOR"); }
    ~GlobalObj() { probe_log("GLOBAL_DTOR"); }
};

GlobalObj g_obj;

extern "C" int probe_func(void)
{
    probe_log("FUNC");
    return 5;
}

extern "C" void probe_global_flags(int *ctor_seen, int *dtor_seen)
{
    if (ctor_seen)
        *ctor_seen = g_global_ctor_done;
    if (dtor_seen)
        *dtor_seen = g_global_dtor_done;
}
