// BUG-30 / G-OVL: the SimpleString::assign shape - five overloads that
// are only DECLARED at the call site, defined further down the TU.  The
// candidate set must come from the class declarations, so the (size_type,
// value_type) overload wins; before the fix this bound to the first
// declaration and failed with "too many arguments to function".
#include <stddef.h>
class S {
public:
    typedef char value_type;
    typedef size_t size_type;
    typedef const char* const_iterator;
    static const size_type npos;
    int m;

    S& assign(const S& s);
    S& assign(const S& s, size_type pos, size_type n);
    S& assign(const value_type* p, size_type n = npos);
    S& assign(size_type n, value_type c);
    S& assign(const_iterator first, const_iterator last);
};
const S::size_type S::npos = size_type(-1);
void probe(S& s)
{
    s.assign((S::size_type)0, (S::value_type)0);
}
S& S::assign(const S& s) { m = 1; return *this; }
S& S::assign(const S& s, size_type pos, size_type n) { m = 2; return *this; }
S& S::assign(const value_type* p, size_type n) { m = 3; return *this; }
S& S::assign(size_type n, value_type c) { m = 4; return *this; }
S& S::assign(const_iterator first, const_iterator last) { m = 5; return *this; }
int main()
{
    S s;
    s.m = 0;
    probe(s);
    if (s.m != 4)
        return s.m == 0 ? 1 : s.m;
    // the other overloads still reach their own bodies
    s.assign(s);
    if (s.m != 1)
        return 10;
    s.assign("abc", (S::size_type)3);
    if (s.m != 3)
        return 11;
    return 0;
}
