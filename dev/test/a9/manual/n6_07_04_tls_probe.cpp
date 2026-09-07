// N6-07-04: TLS touch + worker sync for tcc_delete fail-closed gates.
#include <stdio.h>
#include <windows.h>

extern "C" {
unsigned __cdecl __tcc_cpp_tls_n6_stats(long *out, unsigned max);
int __cdecl __tcc_cpp_tls_n6_cleanup_current_thread(void);
long __cdecl __tcc_cpp_tls_n6_live_tcb_count_read(void);
}

static volatile int g_worker_ready;
static volatile int g_main_proceed;
static volatile int g_allow_worker_touch;

struct TlsObj {
    TlsObj() {}
    ~TlsObj() {}
};
thread_local TlsObj tls;

extern "C" {

static int g_run_cookie = 0x4E363034;

int touch_tls(void)
{
    (void)&tls;
    return 1;
}

int probe_run_cookie(void)
{
    return g_run_cookie;
}

void reset_worker_sync(void)
{
    g_worker_ready = 0;
    g_main_proceed = 0;
    g_allow_worker_touch = 0;
}

void signal_allow_worker_touch(void)
{
    g_allow_worker_touch = 1;
}

DWORD WINAPI worker_hold_live_tls(LPVOID p)
{
    (void)p;
    (void)&tls;
    g_worker_ready = 1;
    while (!g_main_proceed)
        Sleep(1);
    return 0;
}

DWORD WINAPI worker_wait_then_touch(LPVOID p)
{
    (void)p;
    g_worker_ready = 1;
    while (!g_allow_worker_touch)
        Sleep(1);
    touch_tls();
    return 0;
}

}
