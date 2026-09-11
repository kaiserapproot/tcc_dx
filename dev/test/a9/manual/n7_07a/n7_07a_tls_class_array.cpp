// N7-07A TCC-4: thread_local class array (out of N7/N6 scope unless fail-closed).
struct M {
    M() {}
};

thread_local M a[2];

int main()
{
    return 0;
}
