// N6-02 REVIEW FIX-2 fail-closed: 変数側の alignment attribute は runtime へ
// 伝わらないので受理しない.
// 期待: "thread_local with an alignment attribute is unsupported in N6-02"
thread_local int counter __attribute__((aligned(64)));

int main()
{
    return counter;
}
