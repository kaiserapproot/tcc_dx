// N6-02 REVIEW FIX-5 fail-closed: block-scope `thread_local` without `static`
// (C++ では implicit static だが N6-02 subset 外).
// 期待: "thread_local is supported only at namespace scope"
int next_id()
{
    thread_local int counter;
    return ++counter;
}

int main()
{
    return next_id();
}
