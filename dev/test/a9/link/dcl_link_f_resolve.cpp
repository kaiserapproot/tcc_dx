// An extern "C" block sets the linkage of what is declared in it; it does not
// switch off overload resolution for calls written inside it.  TCC used to
// call the most recently declared overload there as is.  winbase.h ran into
// this: InterlockedExchangeAdd on a LONG inside an extern "C" function ran
// the unsigned __int64 overload (a 64-bit locked add on a 32-bit variable).
// The overloads are declared so that "most recent wins" picks a wrong one.
// The exit code names the first wrong pick.
static int f(long volatile *p) { (void)p; return 1; }
static int f(double volatile *p) { (void)p; return 2; }
static int f(unsigned long long volatile *p) { (void)p; return 3; }

static int g(long v) { (void)v; return 1; }
static int g(const char *s) { (void)s; return 2; }

extern "C" {
static int calls_in_block(void)
{
    long l = 0;
    double d = 0;
    unsigned long long u = 0;
    if (f(&l) != 1) return 1;
    if (f(&d) != 2) return 2;
    if (f(&u) != 3) return 3;
    if (g(5L) != 1) return 4;
    if (g("x") != 2) return 5;
    return 0;
}
}

int main()
{
    return calls_in_block();
}
