// BUG-16 再現/回帰: 16 バイト（8 バイト超）の基底サブオブジェクトに対する
// 明示 mem-init `: A(x)` が、基底 ctor の書き込みを本体へ反映すること。
// BUG-15 と同型で、`this` がポインタ化されていないと Win64 ABI で by-value
// 構造体としてコピー渡しされ、基底 ctor の代入が捨てられる。
class A {
public:
    int a, b, c, d;                 // 16 バイト = レジスタ返し/渡し不可.
    A(int x) { a = x; b = x + 1; c = x + 2; d = x + 3; }
};

class D : public A {
public:
    int e;
    D(int x) : A(x) { e = x * 10; }
};

int main()
{
    D o(1);
    // a=1 b=2 c=3 d=4 e=10 -> 合計 20.
    return o.a + o.b + o.c + o.d + o.e - 20;
}
