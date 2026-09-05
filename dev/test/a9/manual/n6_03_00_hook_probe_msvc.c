// N6-03-00: OS-level measurement of the PE TLS callback (IMAGE_TLS_DIRECTORY
// AddressOfCallBacks) as a thread-exit hook in a normal Windows EXE.
// Built with MSVC cl.exe (independent of tcc) to establish the OS Authority
// before any tcc production change.
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>

#ifdef _WIN64
#pragma comment(linker, "/INCLUDE:_tls_used")
#pragma comment(linker, "/INCLUDE:n6_probe_tls_cb_ptr")
#else
#pragma comment(linker, "/INCLUDE:__tls_used")
#pragma comment(linker, "/INCLUDE:_n6_probe_tls_cb_ptr")
#endif

typedef struct probe_tcb {
    DWORD owner_tid;
    int marker;
} probe_tcb;

static DWORD g_slot = TLS_OUT_OF_INDEXES;
static CRITICAL_SECTION g_cs;

#define MAX_LOG 64
typedef struct hook_log {
    DWORD reason;
    DWORD callback_tid;
    probe_tcb *tcb_seen;
    DWORD tcb_owner_tid;
    int tcb_marker;
} hook_log;
static hook_log g_log[MAX_LOG];
static int g_log_count;

static void NTAPI n6_probe_tls_cb(PVOID h, DWORD reason, PVOID pv)
{
    probe_tcb *tcb;
    if (reason != DLL_THREAD_DETACH)
        return;
    tcb = (g_slot != TLS_OUT_OF_INDEXES) ? (probe_tcb *)TlsGetValue(g_slot) : NULL;
    EnterCriticalSection(&g_cs);
    if (g_log_count < MAX_LOG) {
        g_log[g_log_count].reason = reason;
        g_log[g_log_count].callback_tid = GetCurrentThreadId();
        g_log[g_log_count].tcb_seen = tcb;
        g_log[g_log_count].tcb_owner_tid = tcb ? tcb->owner_tid : 0;
        g_log[g_log_count].tcb_marker = tcb ? tcb->marker : 0;
        ++g_log_count;
    }
    LeaveCriticalSection(&g_cs);
}

#pragma const_seg(".CRT$XLB")
EXTERN_C const PIMAGE_TLS_CALLBACK n6_probe_tls_cb_ptr = n6_probe_tls_cb;
#pragma const_seg()

typedef struct worker_arg {
    int use_exit_thread;
    HANDLE go;
    DWORD tid_out;
    probe_tcb *tcb_out;
} worker_arg;

static DWORD WINAPI worker(void *p)
{
    worker_arg *a = (worker_arg *)p;
    probe_tcb *tcb = (probe_tcb *)malloc(sizeof *tcb);
    tcb->owner_tid = GetCurrentThreadId();
    tcb->marker = 0x5A5A0000 | (int)(tcb->owner_tid & 0xFFFF);
    TlsSetValue(g_slot, tcb);
    a->tid_out = tcb->owner_tid;
    a->tcb_out = tcb;
    if (a->go)
        WaitForSingleObject(a->go, INFINITE);
    if (a->use_exit_thread)
        ExitThread(7);
    return 7;
}

static int count_for_tid(DWORD tid, hook_log **first)
{
    int i, n = 0;
    *first = NULL;
    for (i = 0; i < g_log_count; ++i)
        if (g_log[i].callback_tid == tid) {
            if (!*first) *first = &g_log[i];
            ++n;
        }
    return n;
}

static int run_one(const char *name, int use_exit_thread, int base_log)
{
    worker_arg a = {0};
    HANDLE th;
    DWORD rc = 0;
    hook_log *l;
    int n;
    a.use_exit_thread = use_exit_thread;
    th = CreateThread(NULL, 0, worker, &a, 0, NULL);
    WaitForSingleObject(th, INFINITE);
    GetExitCodeThread(th, &rc);
    CloseHandle(th);
    n = count_for_tid(a.tid_out, &l);
    printf("%s: exit_code=%lu hook_count=%d callback_tid_eq_thread_tid=%s tcb_seen=%s tcb_owner_eq_tid=%s marker_ok=%s\n",
           name, rc, n,
           (n && l->callback_tid == a.tid_out) ? "YES" : "NO",
           (n && l->tcb_seen == a.tcb_out) ? "YES" : "NO",
           (n && l->tcb_owner_tid == a.tid_out) ? "YES" : "NO",
           (n && l->tcb_marker == a.tcb_out->marker) ? "YES" : "NO");
    return (n == 1 && l->callback_tid == a.tid_out && l->tcb_seen == a.tcb_out) ? 0 : 1;
}

int main(void)
{
    int fail = 0;
    worker_arg a = {0}, b = {0};
    HANDLE ha, hb;
    hook_log *la, *lb;
    int na, nb, i;
    probe_tcb main_tcb;

    InitializeCriticalSection(&g_cs);
    g_slot = TlsAlloc();
    main_tcb.owner_tid = GetCurrentThreadId();
    main_tcb.marker = 1;
    TlsSetValue(g_slot, &main_tcb);

    fail |= run_one("THREAD_RETURN", 0, 0);
    fail |= run_one("EXITTHREAD", 1, 0);

    // two concurrent workers, both alive at the same time, then released.
    a.go = CreateEvent(NULL, TRUE, FALSE, NULL);
    b.go = a.go;
    ha = CreateThread(NULL, 0, worker, &a, 0, NULL);
    hb = CreateThread(NULL, 0, worker, &b, 0, NULL);
    while (!a.tcb_out || !b.tcb_out) Sleep(1);
    SetEvent(a.go);
    WaitForSingleObject(ha, INFINITE);
    WaitForSingleObject(hb, INFINITE);
    CloseHandle(ha); CloseHandle(hb); CloseHandle(a.go);
    na = count_for_tid(a.tid_out, &la);
    nb = count_for_tid(b.tid_out, &lb);
    printf("TWO_CONCURRENT: A_HOOK_COUNT=%d B_HOOK_COUNT=%d A_HOOK_TID_EQ_A=%s B_HOOK_TID_EQ_B=%s A_TCB_OK=%s B_TCB_OK=%s\n",
           na, nb,
           (na && la->callback_tid == a.tid_out) ? "YES" : "NO",
           (nb && lb->callback_tid == b.tid_out) ? "YES" : "NO",
           (na && la->tcb_seen == a.tcb_out) ? "YES" : "NO",
           (nb && lb->tcb_seen == b.tcb_out) ? "YES" : "NO");
    if (!(na == 1 && nb == 1 && la->tcb_seen == a.tcb_out && lb->tcb_seen == b.tcb_out)) fail = 1;

    // recreate: 3 sequential threads, each must get exactly one hook.
    for (i = 0; i < 3; ++i) {
        char nm[32];
        sprintf(nm, "RECREATE_%d", i);
        fail |= run_one(nm, i & 1, 0);
    }

    // main thread must not have received a DLL_THREAD_DETACH yet.
    {
        hook_log *lm;
        int nm = count_for_tid(main_tcb.owner_tid, &lm);
        printf("MAIN_THREAD_DETACH_HOOKS_SO_FAR=%d\n", nm);
        if (nm != 0) fail = 1;
    }
    printf("TOTAL_DETACH_HOOKS=%d (expected 7)\n", g_log_count);
    if (g_log_count != 7) fail = 1;
    printf("N6_03_00_PE_TLS_CALLBACK_HOOK=%s\n", fail ? "FAIL" : "PASS");
    return fail;
}
