// N6-02 fail-closed: 自身に ctor が無く, member の default ctor を implicit に
// 呼ぶ必要がある型. local/global の既存経路と同じ診断で拒否する.
// 期待: "implicit default construction of non-trivial member is unsupported"
struct M {
    int v;
    M() { v = 1; }
};

struct Outer {
    M m;
    int extra;
};

thread_local Outer object;

int main()
{
    return object.m.v;
}
