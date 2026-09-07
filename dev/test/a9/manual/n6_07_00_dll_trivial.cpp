// N6-07-01: DLL + trivial thread_local scalar (compile-only gate).
extern "C" __declspec(dllexport) int func(void)
{
    thread_local int x;
    return ++x;
}
