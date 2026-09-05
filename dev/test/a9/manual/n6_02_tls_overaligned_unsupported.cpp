// N6-02 REVIEW FIX-2 fail-closed: calloc() の保証 (x64: 16) を超える alignment
// の class は thread_local にできない (N6_02_OVERALIGNED_CLASS=FAIL_CLOSED).
// 期待: "over-aligned thread_local is unsupported in N6-02"
struct __attribute__((aligned(32))) Wide {
    int v;
    Wide() { v = 1; }
};

thread_local Wide wide;

int main()
{
    return wide.v;
}
