// 部分的な mem-init: 明示された基底は書いたとおりに、書かれなかった基底は
// デフォルト ctor で構築されること。明示側の重複呼び出しが無いことも
// カウンタ（A::n）で確認する。
class A {
public:
    int a, n;
    A() { a = 1; n = 1; }
    A(int x) { a = x; n = 1; }      // n=1 固定: 2 回呼ばれれば a が上書きされる.
};

class B {
public:
    int b;
    B() { b = 40; }
};

class D : public A, public B {
public:
    int d;
    D(int x) : A(x) { d = 900; }    // B は暗黙、A は明示.
};

int main()
{
    D o(5);
    // A(5) -> a=5 ; B() -> b=40 ; d=900.
    return o.a + o.b + o.d + o.n - (5 + 40 + 900 + 1);
}
