#ifndef G7_01_INLINE_STRIP_H
#define G7_01_INLINE_STRIP_H

struct S {
    S();
    S(const S &o);
    S(const char *p);
    S &append(const char *p);
};

inline S operator+(const S &a, const char *b)
{
    S result(a);
    result.append(b);
    return result;
}

S make_s();

#endif
