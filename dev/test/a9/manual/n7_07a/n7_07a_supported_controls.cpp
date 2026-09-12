// N7-07A positive controls: supported forms must compile and run cleanly.
#include <stdio.h>

struct Pod {
    int x;
};

struct Trivial {
    static int count;
    Trivial() { ++count; }
};

int Trivial::count;

int main()
{
    Pod pods[4];
    Trivial one;

    pods[0].x = 0;
    printf("SUPPORTED_CONTROL count=%d\n", Trivial::count);
    return (Trivial::count == 1) ? 0 : 1;
}
