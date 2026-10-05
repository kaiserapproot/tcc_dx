/* Values of the bit scan intrinsics and __rdtsc (TCC bodies in
   dev\include\psdk_inc\intrin-tcc.h).  Built as C here and as C++ by
   sdk_bitscan_rdtsc_cpp.cpp.  The exit code names the first failure. */
#include <windows.h>
#include <intrin.h>

#define CHECK(code, cond) do { if (!(cond)) return (code); } while (0)

/* reference results computed with plain C */
static int ref_lowest(unsigned __int64 m)
{
    int i;
    for (i = 0; i < 64; i++)
        if ((m >> i) & 1) return i;
    return -1;
}

static int ref_highest(unsigned __int64 m)
{
    int i;
    for (i = 63; i >= 0; i--)
        if ((m >> i) & 1) return i;
    return -1;
}

static unsigned __int64 g_seed = 0x9E3779B97F4A7C15ULL;
static unsigned __int64 next_rand(void)
{
    /* xorshift64: fixed seed, so every run checks the same values */
    g_seed ^= g_seed << 13;
    g_seed ^= g_seed >> 7;
    g_seed ^= g_seed << 17;
    return g_seed;
}

static int check_zero(void)
{
    unsigned long idx = 77;
    /* Mask 0: result 0 and Index left alone (MSVC leaves it undefined) */
    CHECK(1, _BitScanForward(&idx, 0) == 0 && idx == 77);
    CHECK(2, _BitScanReverse(&idx, 0) == 0 && idx == 77);
    CHECK(3, _BitScanForward64(&idx, 0) == 0 && idx == 77);
    CHECK(4, _BitScanReverse64(&idx, 0) == 0 && idx == 77);
    return 0;
}

static int check_single_bits(void)
{
    unsigned long idx;
    int i;
    for (i = 0; i < 32; i++) {
        unsigned long m = 1UL << i;
        idx = 99;
        CHECK(11, _BitScanForward(&idx, m) == 1 && idx == (unsigned long)i);
        idx = 99;
        CHECK(12, _BitScanReverse(&idx, m) == 1 && idx == (unsigned long)i);
    }
    for (i = 0; i < 64; i++) {
        unsigned __int64 m = 1ULL << i;
        idx = 99;
        CHECK(13, _BitScanForward64(&idx, m) == 1 && idx == (unsigned long)i);
        idx = 99;
        CHECK(14, _BitScanReverse64(&idx, m) == 1 && idx == (unsigned long)i);
    }
    return 0;
}

static int check_patterns(void)
{
    unsigned long idx;
    int k;
    /* both ends set */
    CHECK(21, _BitScanForward(&idx, 0x80000001UL) == 1 && idx == 0);
    CHECK(22, _BitScanReverse(&idx, 0x80000001UL) == 1 && idx == 31);
    CHECK(23, _BitScanForward64(&idx, 0x8000000000000001ULL) == 1 && idx == 0);
    CHECK(24, _BitScanReverse64(&idx, 0x8000000000000001ULL) == 1 && idx == 63);
    /* all set */
    CHECK(25, _BitScanReverse(&idx, 0xFFFFFFFFUL) == 1 && idx == 31);
    CHECK(26, _BitScanReverse64(&idx, 0xFFFFFFFFFFFFFFFFULL) == 1 && idx == 63);
    /* the 64-bit forms must look at the high half */
    CHECK(27, _BitScanForward64(&idx, 0xF000000000000000ULL) == 1 && idx == 60);
    CHECK(28, _BitScanReverse64(&idx, 0x00000001F0000000ULL) == 1 && idx == 32);
    /* 100000 pseudo-random masks against the C reference */
    for (k = 0; k < 100000; k++) {
        unsigned __int64 m = next_rand() >> (k & 63);
        unsigned long m32 = (unsigned long)m;
        if (m) {
            CHECK(31, _BitScanForward64(&idx, m) == 1 && (int)idx == ref_lowest(m));
            CHECK(32, _BitScanReverse64(&idx, m) == 1 && (int)idx == ref_highest(m));
        }
        if (m32) {
            CHECK(33, _BitScanForward(&idx, m32) == 1 && (int)idx == ref_lowest(m32));
            CHECK(34, _BitScanReverse(&idx, m32) == 1 && (int)idx == ref_highest(m32));
        }
    }
    /* the winnt.h names are the same functions */
    CHECK(35, BitScanForward(&idx, 0x40UL) == 1 && idx == 6);
    CHECK(36, BitScanReverse64(&idx, 0x40ULL << 32) == 1 && idx == 38);
    return 0;
}

/* RDTSC is not serializing, and counters of different CPUs need not agree
   (Microsoft notes negative deltas when a thread moves between CPUs whose
   TSCs are not in sync).  So the ordering checks run with the thread pinned
   to one logical CPU, and the affinity is restored afterwards. */
static int check_rdtsc_pinned(void)
{
    unsigned __int64 t0, t1, t2;
    int i;
    t0 = __rdtsc();
    CHECK(41, t0 != 0);
    /* never goes backwards on one thread, and it really counts */
    for (i = 0; i < 1000; i++) {
        t1 = __rdtsc();
        CHECK(42, t1 >= t0);
        t0 = t1;
    }
    Sleep(20);
    t2 = ReadTimeStampCounter();
    CHECK(43, t2 > t1);
    /* the high half is used: a counter only in eax would wrap every few
       seconds and would not reach this size on a machine that has been up
       for more than a moment */
    CHECK(44, (t2 >> 32) != 0);
    return 0;
}

static int check_rdtsc(void)
{
    DWORD_PTR process_mask, system_mask, one_cpu, old_mask;
    int rc;
    CHECK(45, GetProcessAffinityMask(GetCurrentProcess(), &process_mask, &system_mask) && process_mask != 0);
    one_cpu = process_mask & (~process_mask + 1);   /* lowest CPU this process may use */
    old_mask = SetThreadAffinityMask(GetCurrentThread(), one_cpu);
    CHECK(46, old_mask != 0);
    rc = check_rdtsc_pinned();
    SetThreadAffinityMask(GetCurrentThread(), old_mask);
    return rc;
}

int main(void)
{
    int rc;
    if ((rc = check_zero()) != 0) return rc;
    if ((rc = check_single_bits()) != 0) return rc;
    if ((rc = check_patterns()) != 0) return rc;
    if ((rc = check_rdtsc()) != 0) return rc;
    return 0;
}
