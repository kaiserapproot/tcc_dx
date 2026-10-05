/* Fences, pause and compiler barriers (TCC bodies in
   dev\include\psdk_inc\intrin-tcc.h).  Built as C here and as C++ by
   sdk_fence_cpp.cpp.  The exit code names the first failure.

   1. Each body holds the right instruction (read from its code bytes), so
      lfence / mfence / sfence cannot be swapped unnoticed.
   2. Store-buffer litmus test: two threads each store to their own variable,
      fence, then load the other one.  On x86 a store may still sit in the
      store buffer when the later load runs, so without a full fence both
      loads can see 0.  With _mm_mfence, __faststorefence or MemoryBarrier
      that must never happen.
   Run with the argument "control" to print how often it does happen with no
   fence and with sfence (mingw's __faststorefence); the gate does not use
   those counts because they depend on the machine. */
#include <windows.h>
#include <intrin.h>
#include <stdio.h>
#include <string.h>

#define CHECK(code, cond) do { if (!(cond)) return (code); } while (0)

typedef void (*fence_fn)(void);

/* 1. instructions */

static int has_bytes(fence_fn f, const unsigned char *pat, int len)
{
    const unsigned char *p = (const unsigned char *)(void *)f;
    int i, end;
    /* The body is a prologue, the instruction and "leave; ret" (C9 C3).
       Look only up to there: the next function follows right after. */
    for (end = 0; end < 64; end++)
        if (p[end] == 0xC9 && p[end + 1] == 0xC3)
            break;
    for (i = 0; i + len <= end; i++)
        if (memcmp(p + i, pat, len) == 0)
            return 1;
    return 0;
}

static int check_opcodes(void)
{
    static const unsigned char lfence[] = { 0x0F, 0xAE, 0xE8 };
    static const unsigned char mfence[] = { 0x0F, 0xAE, 0xF0 };
    static const unsigned char sfence[] = { 0x0F, 0xAE, 0xF8 };
    static const unsigned char pause_[] = { 0xF3, 0x90 };
    static const unsigned char lock_or_rsp[] = { 0xF0, 0x83, 0x0C, 0x24, 0x00 };
    CHECK(1, has_bytes(_mm_lfence, lfence, 3) && !has_bytes(_mm_lfence, mfence, 3));
    CHECK(2, has_bytes(_mm_mfence, mfence, 3) && !has_bytes(_mm_mfence, lfence, 3));
    CHECK(3, has_bytes(_mm_sfence, sfence, 3) && !has_bytes(_mm_sfence, mfence, 3));
    CHECK(4, has_bytes(_mm_pause, pause_, 2));
    CHECK(5, has_bytes(__faststorefence, lock_or_rsp, 5) && !has_bytes(__faststorefence, sfence, 3));
    return 0;
}

/* every spelling runs (the winnt.h names are macros onto the intrinsics) */
static int check_calls(void)
{
    volatile LONG v = 0;
    _mm_lfence();
    _mm_mfence();
    _mm_sfence();
    _mm_pause();
    __faststorefence();
    MemoryBarrier();
    MemoryFence();
    LoadFence();
    StoreFence();
    FastFence();
    YieldProcessor();
    v = 1;
    _ReadWriteBarrier();
    _ReadBarrier();
    _WriteBarrier();
    CHECK(11, v == 1);
    return 0;
}

/* 2. store-buffer litmus */

#define ROUNDS 100000

static volatile LONG g_x, g_y, g_r1, g_r2;
static volatile LONG g_go;      /* round number published by main */
static volatile LONG g_done;    /* threads finished with this round */
static volatile LONG g_stop;
static fence_fn g_fence;

static void no_fence(void) { }
static void memory_barrier(void) { MemoryBarrier(); }

static DWORD WINAPI side_a(LPVOID arg)
{
    LONG seen = 0;
    (void)arg;
    for (;;) {
        while (g_go == seen)
            if (g_stop) return 0;
        seen = g_go;
        g_x = 1;
        g_fence();
        g_r1 = g_y;
        InterlockedIncrement(&g_done);
    }
}

static DWORD WINAPI side_b(LPVOID arg)
{
    LONG seen = 0;
    (void)arg;
    for (;;) {
        while (g_go == seen)
            if (g_stop) return 0;
        seen = g_go;
        g_y = 1;
        g_fence();
        g_r2 = g_x;
        InterlockedIncrement(&g_done);
    }
}

/* returns the number of rounds where both loads saw 0, or -1 on a stall */
static long litmus(fence_fn f)
{
    HANDLE h[2];
    long both_zero = 0;
    long round;
    DWORD start;
    g_fence = f;
    g_stop = 0;
    g_go = 0;
    h[0] = CreateThread(NULL, 0, side_a, NULL, 0, NULL);
    h[1] = CreateThread(NULL, 0, side_b, NULL, 0, NULL);
    if (!h[0] || !h[1]) return -1;
    start = GetTickCount();
    for (round = 1; round <= ROUNDS; round++) {
        g_x = 0;
        g_y = 0;
        g_done = 0;
        InterlockedExchange(&g_go, round);   /* full barrier, then publish */
        while (g_done != 2) {
            if (GetTickCount() - start > 60000) {
                g_stop = 1;
                return -1;
            }
        }
        if (g_r1 == 0 && g_r2 == 0)
            both_zero++;
    }
    g_stop = 1;
    WaitForMultipleObjects(2, h, TRUE, 10000);
    CloseHandle(h[0]);
    CloseHandle(h[1]);
    return both_zero;
}

static int check_litmus(void)
{
    CHECK(21, litmus(_mm_mfence) == 0);
    CHECK(22, litmus(__faststorefence) == 0);
    CHECK(23, litmus(memory_barrier) == 0);
    return 0;
}

int main(int argc, char **argv)
{
    int rc;
    if (argc > 1 && strcmp(argv[1], "control") == 0) {
        printf("both loads saw 0 in %d rounds: no fence %ld, sfence %ld, mfence %ld, __faststorefence %ld\n",
               ROUNDS, litmus(no_fence), litmus(_mm_sfence), litmus(_mm_mfence), litmus(__faststorefence));
        return 0;
    }
    if ((rc = check_opcodes()) != 0) return rc;
    if ((rc = check_calls()) != 0) return rc;
    if ((rc = check_litmus()) != 0) return rc;
    return 0;
}
