/* SysV x86-64: a function returning through the hidden pointer must hand
   that pointer back in %rax.  C++ destructors run after the result is
   stored (a local with a destructor, or the class's own destructor on the
   local being returned) used to clobber it.  tcc callers never read %rax
   here, so the call goes through asm and checks the register directly. */
#include <string.h>

struct D8  { long a; ~D8() {} };
struct D24 { long a, b, c; ~D24() {} };
struct C24 { long a, b, c; };
struct Guard { long g; ~Guard() { g = 0; } };
/* implicit destructor only (through the member): returned through the
   hidden pointer because of the fix this branch is stacked on */
static int member_dtors;
struct Member { long value; ~Member() { ++member_dtors; } };
struct Result { Member member; long pad; };

D8 ret_d8(long v) { D8 r; r.a = v; return r; }
D24 ret_d24(long v) { D24 r; r.a = v; r.b = v + 1; r.c = v + 2; return r; }
C24 ret_c24_guarded(long v)
{
    Guard g;
    C24 r;
    g.g = v;
    r.a = v; r.b = v + 1; r.c = v + 2;
    return r;
}
C24 ret_c24_two_paths(long v)
{
    Guard g;
    C24 r;
    g.g = v;
    r.a = r.b = r.c = v;
    if (v > 100) {
        Guard inner;
        inner.g = v;
        return r;
    }
    return r;
}
Result ret_result(long v)
{
    Result r;
    r.member.value = v;
    r.pad = v + 1;
    return r;
}

static void *call_sret(void *fn, void *buf, long v)
{
    void *ret;
    __asm__ __volatile__(
        "mov %1, %%rdi\n\t"
        "mov %2, %%rsi\n\t"
        "call *%3\n\t"
        : "=a"(ret)
        : "m"(buf), "m"(v), "m"(fn)
        : "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory");
    return ret;
}

int main()
{
    long buf[8];
    memset(buf, 0, sizeof buf);
    if (call_sret((void *)ret_d24, buf, 10) != buf || buf[2] != 12)
        return 1;
    if (call_sret((void *)ret_c24_guarded, buf, 20) != buf || buf[2] != 22)
        return 2;
    if (call_sret((void *)ret_c24_two_paths, buf, 1) != buf || buf[0] != 1)
        return 3;
    if (call_sret((void *)ret_c24_two_paths, buf, 200) != buf || buf[0] != 200)
        return 4;
    /* a class with a destructor is returned through memory even at 8 bytes */
    if (call_sret((void *)ret_d8, buf, 30) != buf || buf[0] != 30)
        return 5;
    /* implicit destructor: hidden pointer in %rax, values intact, and the
       callee destroys its own objects a fixed number of times per call
       (at most once: no double destruction; zero would be legal with
       return value optimisation) */
    {
        int before = member_dtors, per_call;
        if (call_sret((void *)ret_result, buf, 40) != buf
            || buf[0] != 40 || buf[1] != 41)
            return 6;
        per_call = member_dtors - before;
        if (per_call < 0 || per_call > 1)
            return 7;
        before = member_dtors;
        if (call_sret((void *)ret_result, buf, 50) != buf || buf[0] != 50)
            return 8;
        if (member_dtors - before != per_call)
            return 9;
    }
    return 0;
}
