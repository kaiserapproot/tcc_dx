// Overloads that differ only in signedness (long / unsigned long,
// long long / unsigned long long) or in the qualifiers of what a pointer
// points to.  Two bugs used to break them:
//  - the link name used 'l' for long and unsigned long, 'L' for long long and
//    unsigned long long, and dropped const / volatile under a pointer, so such
//    overloads got one symbol ("defined twice")
//  - T* -> volatile T* was ranked like any pointer conversion, so for a long*
//    argument f(long volatile *) tied with f(double volatile *) and the most
//    recently declared overload won.  winbase.h hit this: InterlockedIncrement
//    on a LONG picked the unsigned __int64 overload.
// Some functions are declared before main and defined after it, so the
// declaration and the definition must agree on the link name.
// The exit code names the first wrong pick.

int by_val(long v);
int by_val(unsigned long v);
int by_val(long long v);
int by_val(unsigned long long v);

int by_ptr(long *p);
int by_ptr(const long *p);
int by_ptr(volatile long *p);
int by_ptr(unsigned long *p);

// all parameters volatile, as in winbase.h; the most specific one is
// declared first so that "last declared wins" would pick a wrong one
int vol_only(long volatile *p) { (void)p; return 1; }
int vol_only(unsigned long volatile *p) { (void)p; return 2; }
int vol_only(unsigned long long volatile *p) { (void)p; return 3; }
int vol_only(double volatile *p) { (void)p; return 4; }

int const_only(const char *s) { (void)s; return 1; }
int const_only(const unsigned char *s) { (void)s; return 2; }
int const_only(const double *s) { (void)s; return 3; }

int main()
{
    long l = 0;
    unsigned long ul = 0;
    long long ll = 0;
    unsigned long long ull = 0;
    const long cl = 0;
    volatile long vl = 0;
    char ch = 0;
    unsigned char uch = 0;

    if (by_val(l) != 1) return 1;
    if (by_val(ul) != 2) return 2;
    if (by_val(ll) != 3) return 3;
    if (by_val(ull) != 4) return 4;

    if (by_ptr(&l) != 1) return 5;
    if (by_ptr(&cl) != 2) return 6;
    if (by_ptr(&vl) != 3) return 7;
    if (by_ptr(&ul) != 4) return 8;

    if (vol_only(&l) != 1) return 9;
    if (vol_only(&ul) != 2) return 10;
    if (vol_only(&ull) != 3) return 11;
    if (vol_only(&vl) != 1) return 12;

    if (const_only(&ch) != 1) return 13;
    if (const_only(&uch) != 2) return 14;
    return 0;
}

int by_val(long v) { (void)v; return 1; }
int by_val(unsigned long v) { (void)v; return 2; }
int by_val(long long v) { (void)v; return 3; }
int by_val(unsigned long long v) { (void)v; return 4; }

int by_ptr(long *p) { (void)p; return 1; }
int by_ptr(const long *p) { (void)p; return 2; }
int by_ptr(volatile long *p) { (void)p; return 3; }
int by_ptr(unsigned long *p) { (void)p; return 4; }
