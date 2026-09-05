// N6-02 REVIEW FIX-5 fail-closed: CLASS_STATIC_THREAD_LOCAL.
// 期待: "static thread_local is unsupported in N6-02"
struct Registry {
    static thread_local int per_thread_count;
};

int main()
{
    return Registry::per_thread_count;
}
