// N6-02 REVIEW FIX-5 fail-closed: FUNCTION_STATIC_THREAD_LOCAL.
// 期待: "static thread_local is unsupported in N6-02"
int next_id()
{
    static thread_local int counter;
    return ++counter;
}

int main()
{
    return next_id();
}
