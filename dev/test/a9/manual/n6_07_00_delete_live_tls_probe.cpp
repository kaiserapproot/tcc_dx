// N6-07-04: direct relocate TLS touch + registry export for delete-live audit.
#include <stdio.h>
#include <stddef.h>

extern "C" {
unsigned __cdecl __tcc_cpp_tls_n6_registry_snapshot(void **objects,
    void **dtors, unsigned max);
}

static volatile int g_ctor;
static volatile int g_dtor;
static void *g_snapshot_objects[4];
static void *g_snapshot_dtors[4];

struct LiveTls {
    LiveTls() { ++g_ctor; }
    ~LiveTls() { ++g_dtor; }
};
thread_local LiveTls tls;

extern "C" {

int touch_live_tls(void)
{
    (void)&tls;
    return 7;
}

unsigned int probe_live_tls_registry_count(void)
{
    return __tcc_cpp_tls_n6_registry_snapshot(g_snapshot_objects, g_snapshot_dtors, 4);
}

unsigned long long probe_live_tls_registry_obj0_u64(void)
{
    unsigned int n;

    n = __tcc_cpp_tls_n6_registry_snapshot(g_snapshot_objects, g_snapshot_dtors, 4);
    if (n == 0)
        return 0;
    return (unsigned long long)(size_t)g_snapshot_objects[0];
}

unsigned long long probe_live_tls_registry_dtor0_u64(void)
{
    unsigned int n;

    n = __tcc_cpp_tls_n6_registry_snapshot(g_snapshot_objects, g_snapshot_dtors, 4);
    if (n == 0)
        return 0;
    return (unsigned long long)(size_t)g_snapshot_dtors[0];
}

int probe_dtor_ptr_inside_run_range(unsigned long long lo, unsigned long long hi)
{
    unsigned long long dtor;
    unsigned int n;

    n = __tcc_cpp_tls_n6_registry_snapshot(g_snapshot_objects, g_snapshot_dtors, 4);
    if (n == 0)
        return 0;
    dtor = (unsigned long long)(size_t)g_snapshot_dtors[0];
    if (dtor >= lo && dtor < hi)
        return 1;
    return 0;
}

int probe_obj_ptr_inside_run_range(unsigned long long lo, unsigned long long hi)
{
    unsigned long long obj;
    unsigned int n;

    n = __tcc_cpp_tls_n6_registry_snapshot(g_snapshot_objects, g_snapshot_dtors, 4);
    if (n == 0)
        return 0;
    obj = (unsigned long long)(size_t)g_snapshot_objects[0];
    if (obj >= lo && obj < hi)
        return 1;
    return 0;
}

}
