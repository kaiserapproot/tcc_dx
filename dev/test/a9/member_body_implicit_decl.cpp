// An identifier that is not a member of the enclosing class must fall through
// to the ordinary lookup, not be reported as a missing field.
//
// The BUG-21 class-scope-first probe used the erroring cpp_lookup_member_field()
// for its `!s` case, so an undeclared function called from a member body died
// with "field not found: <name>" instead of taking the implicit-declaration
// path that the very same call takes in a free function.  amateras cross.h hit
// this on snprintf() inside window_t::reg_window() as soon as CROSS_DISABLE_MMD
// removed the earlier global call that had implicitly declared it.
//
// Constraints this file has to satisfy, all of them load-bearing:
//   - <stdio.h> is NOT included, so putchar is undeclared in this TU; it still
//     resolves against the CRT at link time, so the test runs and exits 0.
//   - putchar appears ONLY inside the member body.  Any earlier use (a free
//     function, or a call from main) implicitly declares it first and the
//     deferred member body then finds it - that is exactly why cross.h
//     compiled as long as the MMD block was enabled.
//   - main() calls S::m, because member bodies are only compiled when used.
//
// Pre-fix (a3eba99): error "field not found: putchar".
// Post-fix: warning "implicit declaration of function 'putchar'", exit 0.

struct S {
    int m(int c) { return putchar(c); }
};

int main(void)
{
    S s;
    s.m('m');
    s.m('\n');
    return 0;
}
