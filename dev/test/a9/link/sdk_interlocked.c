/* Values of every Interlocked intrinsic that has a TCC body
   (dev\include\psdk_inc\intrin-tcc.h): the return value and the memory after
   the call.  Built as C here and as C++ by sdk_interlocked_cpp.cpp.
   Only <windows.h> is included on purpose: the intrinsics must be usable
   without <intrin.h>, as with MSVC.  The exit code names the first failure. */
#include <windows.h>

#define CHECK(code, cond) do { if (!(cond)) return (code); } while (0)

static int check_32(void)
{
    LONG v = 10;
    CHECK(1, InterlockedIncrement(&v) == 11 && v == 11);
    CHECK(2, InterlockedDecrement(&v) == 10 && v == 10);
    CHECK(3, InterlockedExchange(&v, 5) == 10 && v == 5);
    CHECK(4, InterlockedExchangeAdd(&v, 4) == 5 && v == 9);
    CHECK(5, InterlockedAdd(&v, 3) == 12 && v == 12);
    CHECK(6, InterlockedCompareExchange(&v, 7, 12) == 12 && v == 7);
    CHECK(7, InterlockedCompareExchange(&v, 1, 12) == 7 && v == 7);
    v = 0x0F0F;
    CHECK(8, InterlockedAnd(&v, 0x00FF) == 0x0F0F && v == 0x000F);
    CHECK(9, InterlockedOr(&v, 0x0100) == 0x000F && v == 0x010F);
    CHECK(10, InterlockedXor(&v, 0x0101) == 0x010F && v == 0x000E);
    /* sign and wrap-around are the CPU's, not the compiler's */
    v = 0x7FFFFFFF;
    CHECK(11, InterlockedIncrement(&v) == (LONG)0x80000000 && v == (LONG)0x80000000);
    v = 0;
    CHECK(12, InterlockedDecrement(&v) == -1 && v == -1);
    /* the leading-underscore names are the same functions */
    v = 1;
    CHECK(13, _InterlockedIncrement(&v) == 2 && _InterlockedCompareExchange(&v, 3, 2) == 2 && v == 3);
    return 0;
}

static int check_64(void)
{
    LONG64 w = 0x100000000LL;
    CHECK(21, InterlockedIncrement64(&w) == 0x100000001LL && w == 0x100000001LL);
    CHECK(22, InterlockedDecrement64(&w) == 0x100000000LL && w == 0x100000000LL);
    CHECK(23, InterlockedExchange64(&w, 0x500000000LL) == 0x100000000LL && w == 0x500000000LL);
    CHECK(24, InterlockedExchangeAdd64(&w, 0x400000000LL) == 0x500000000LL && w == 0x900000000LL);
    CHECK(25, InterlockedAdd64(&w, 3) == 0x900000003LL && w == 0x900000003LL);
    CHECK(26, InterlockedCompareExchange64(&w, 7, 0x900000003LL) == 0x900000003LL && w == 7);
    CHECK(27, InterlockedCompareExchange64(&w, 1, 0x900000003LL) == 7 && w == 7);
    w = 0x0F0F00000000LL;
    CHECK(28, InterlockedAnd64(&w, 0x00FF00000000LL) == 0x0F0F00000000LL && w == 0x000F00000000LL);
    CHECK(29, InterlockedOr64(&w, 0x010000000000LL) == 0x000F00000000LL && w == 0x010F00000000LL);
    CHECK(30, InterlockedXor64(&w, 0x010100000000LL) == 0x010F00000000LL && w == 0x000E00000000LL);
    /* a carry out of the low 32 bits must reach the high half */
    w = 0xFFFFFFFFLL;
    CHECK(31, InterlockedIncrement64(&w) == 0x100000000LL && w == 0x100000000LL);
    return 0;
}

static int check_16(void)
{
    SHORT s = 0x7FFF;
    CHECK(41, InterlockedIncrement16(&s) == (SHORT)0x8000 && s == (SHORT)0x8000);
    CHECK(42, InterlockedDecrement16(&s) == 0x7FFF && s == 0x7FFF);
    CHECK(43, InterlockedCompareExchange16(&s, 5, 0x7FFF) == 0x7FFF && s == 5);
    CHECK(44, InterlockedCompareExchange16(&s, 1, 0x7FFF) == 5 && s == 5);
    return 0;
}

/* The 16-bit forms must touch only their two bytes. */
static int check_16_neighbours(void)
{
    union { SHORT s[4]; LONG64 all; } u;
    u.all = 0x1111222233334444LL;
    InterlockedIncrement16(&u.s[1]);
    CHECK(51, u.all == 0x1111222233344444LL);
    InterlockedCompareExchange16(&u.s[2], 0x7777, 0x2222);
    CHECK(52, u.all == 0x1111777733344444LL);
    return 0;
}

static int check_pointer(void)
{
    int a = 1, b = 2;
    PVOID p = &a;
    CHECK(61, InterlockedExchangePointer(&p, &b) == (PVOID)&a && p == (PVOID)&b);
    CHECK(62, InterlockedCompareExchangePointer(&p, &a, &b) == (PVOID)&b && p == (PVOID)&a);
    CHECK(63, InterlockedCompareExchangePointer(&p, NULL, &b) == (PVOID)&a && p == (PVOID)&a);
    return 0;
}

int main(void)
{
    int rc;
    if ((rc = check_32()) != 0) return rc;
    if ((rc = check_64()) != 0) return rc;
    if ((rc = check_16()) != 0) return rc;
    if ((rc = check_16_neighbours()) != 0) return rc;
    if ((rc = check_pointer()) != 0) return rc;
    return 0;
}
