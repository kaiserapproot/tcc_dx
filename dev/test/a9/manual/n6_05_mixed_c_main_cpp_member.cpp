// N6-05: C global main + C++ TU with member function named main (mixed link).
#include <stdio.h>

class X {
public:
    void main() {}
};

struct Tls {
    Tls() {}
    ~Tls() { printf("TLS_DTOR\n"); fflush(stdout); }
};
thread_local Tls tls;

extern "C" void touch_cpp_tls()
{
    (void)&tls;
}
