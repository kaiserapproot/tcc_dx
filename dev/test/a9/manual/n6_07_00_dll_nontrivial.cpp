// N6-07-01: DLL + non-trivial thread_local dtor (compile-only gate).
struct T {
    T() {}
    ~T() {}
};
thread_local T x;

extern "C" __declspec(dllexport) int func(void)
{
    (void)&x;
    return 1;
}
