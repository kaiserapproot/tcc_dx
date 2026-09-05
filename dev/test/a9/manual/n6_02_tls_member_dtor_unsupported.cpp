// N6-02 fail-closed: 自身は dtor を宣言しないが member が non-trivial dtor を
// 持つ型 (implicit destructor が non-trivial) も受理しない.
// 期待: "thread_local object with non-trivial destructor is unsupported in N6-02"
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
