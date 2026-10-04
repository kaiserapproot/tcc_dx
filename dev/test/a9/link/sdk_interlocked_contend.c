/* Atomicity, not just values: several threads hit the same variables at once.
   A read-modify-write without the lock prefix loses updates under contention,
   so the totals below come out short or the spin lock never comes free.
   Built as C here and as C++ by sdk_interlocked_contend_cpp.cpp.
   The exit code names the first wrong total. */
#include <windows.h>

#define THREADS 4
#define LOOPS 500000

static volatile LONG g_inc;
static volatile LONG64 g_add64;
static volatile SHORT g_inc16;
static volatile LONG g_dec;
static volatile LONG g_lock;
static volatile LONG g_guarded;
static volatile LONG g_or;
static volatile LONG g_xor;
static volatile LONG64 g_and64;
static PVOID volatile g_ptr_lock;
static volatile LONG g_ptr_guarded;
static volatile LONG g_ptr_release_fail;
static HANDLE g_start;
static char g_tokens[THREADS];

static DWORD WINAPI worker(LPVOID arg)
{
    int id = (int)(INT_PTR)arg;
    int i;
    WaitForSingleObject(g_start, INFINITE);
    for (i = 0; i < LOOPS; i++) {
        PVOID mine = &g_tokens[id];
        InterlockedIncrement(&g_inc);
        InterlockedExchangeAdd64(&g_add64, 0x100000001LL);
        InterlockedIncrement16(&g_inc16);
        InterlockedDecrement(&g_dec);
        /* spin lock built from CompareExchange / Exchange guards a plain add */
        while (InterlockedCompareExchange(&g_lock, 1, 0) != 0) { }
        g_guarded = g_guarded + 1;
        InterlockedExchange(&g_lock, 0);
        /* each thread owns one bit: Or sets it, Xor flips it an even number
           of times, And clears another thread's bit in a 64-bit word only if
           the loop is the last one */
        InterlockedOr(&g_or, 1L << id);
        InterlockedXor(&g_xor, 1L << id);
        if (i == LOOPS - 1)
            InterlockedAnd64(&g_and64, ~(1LL << (id + 32)));
        /* the same kind of lock built from the pointer CompareExchange alone:
           taken by swapping NULL for this thread's token, released by swapping
           the token back for NULL.  A non-atomic pointer CAS lets two threads
           in at once and the plain add below comes out short. */
        while (InterlockedCompareExchangePointer(&g_ptr_lock, mine, NULL) != NULL) { }
        g_ptr_guarded = g_ptr_guarded + 1;
        if (InterlockedCompareExchangePointer(&g_ptr_lock, NULL, mine) != mine)
            InterlockedIncrement(&g_ptr_release_fail);
    }
    return 0;
}

int main(void)
{
    HANDLE h[THREADS];
    int i;
    LONG expect_or = 0;
    g_and64 = 0x0000000F0000000FLL;
    g_dec = THREADS * LOOPS;
    g_start = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (!g_start) return 100;
    for (i = 0; i < THREADS; i++) {
        h[i] = CreateThread(NULL, 0, worker, (LPVOID)(INT_PTR)i, 0, NULL);
        if (!h[i]) return 101;
        expect_or |= 1L << i;
    }
    SetEvent(g_start);
    /* A broken (non-atomic) CompareExchange can leave the spin lock taken for
       good; fail instead of hanging the gate.  Returning from main ends the
       process and its threads. */
    if (WaitForMultipleObjects(THREADS, h, TRUE, 60000) != WAIT_OBJECT_0) return 99;
    if (g_inc != THREADS * LOOPS) return 1;
    if (g_add64 != (LONG64)THREADS * LOOPS * 0x100000001LL) return 2;
    if (g_inc16 != (SHORT)(THREADS * LOOPS)) return 3;
    if (g_dec != 0) return 4;
    if (g_guarded != THREADS * LOOPS) return 5;
    if (g_or != expect_or) return 6;
    if (g_xor != 0) return 7;
    if (g_and64 != 0x000000000000000FLL) return 8;
    if (g_ptr_guarded != THREADS * LOOPS) return 9;
    if (g_ptr_lock != NULL || g_ptr_release_fail != 0) return 10;
    return 0;
}
