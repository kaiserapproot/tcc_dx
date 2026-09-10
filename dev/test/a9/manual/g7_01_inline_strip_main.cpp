#include "g7_01_inline_strip.h"

S::S() {}
S::S(const S &o) { (void)o; }
S::S(const char *p) { (void)p; }
S &S::append(const char *p) { (void)p; return *this; }
S make_s() { S x("b"); return x; }

int main()
{
    S x = make_s() + "x";
    (void)x;
    return 0;
}
