// N6-02/N6-04 fail-closed: 自身は dtor を宣言しないが member が non-trivial dtor を
// 持つ型 (implicit destructor が non-trivial) は N6-04 でも受理しない
// (dtor thunk の呼び先となる user destructor がない. global 経路と同じ制限).
// 期待: "thread_local object with implicit non-trivial destructor is unsupported in N6-04"
struct M {
    int v;
    M() { v = 1; }
    ~M() { v = 0; }
};

struct Outer {
    M m;
    Outer() {}
};

thread_local Outer object;

int main()
{
    return object.m.v;
}
