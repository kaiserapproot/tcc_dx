// N6-02 REVIEW FIX-5 fail-closed: EXTERN_THREAD_LOCAL (cross-TU descriptor は
// N6-02 に存在しない).
// 期待: "extern thread_local is unsupported in N6-02"
extern thread_local int shared_counter;

int main()
{
    return shared_counter;
}
