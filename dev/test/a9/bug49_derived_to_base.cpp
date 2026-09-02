// BUG-49: a derived-to-base conversion must not run the base copy ctor
// repeatedly.  cpp_try_class_conversion() used to compare the operand type
// with `ref == class_sym` only, so a derived operand entered the
// converting-ctor search; the chosen `B(const B&)` then converted its own
// `const B&` parameter the same way and recursed until cpp_conv_depth cut
// it off.  Measured before the fix: `B x(d);` gave 501 instead of 101, and
// binding to `const B&` built a temporary (401) where C++98 copies nothing
// at all - which also meant writes through a `B&` were lost.
#include <stdio.h>

static int g_copies = 0;

struct B {
    int b;
    B() { b = 1; }
    B(const B& o) { b = o.b + 100; g_copies++; }
};

struct D : public B {
    int d;
    D() { d = 2; }
};

static int peek_const_ref(const B& x)
{
    return x.b;
}

static void write_through_ref(B& x)
{
    x.b = 9;
}

int main()
{
    D x;
    B di(x);

    printf("direct=%d copies=%d\n", di.b, g_copies);
    if (di.b != 101)
        return 1;
    if (g_copies != 1)
        return 2;
    if (peek_const_ref(x) != 1)
        return 3;
    if (g_copies != 1)
        return 4;
    write_through_ref(x);
    printf("after write=%d copies=%d\n", x.b, g_copies);
    if (x.b != 9)
        return 5;
    if (g_copies != 1)
        return 6;
    return 0;
}