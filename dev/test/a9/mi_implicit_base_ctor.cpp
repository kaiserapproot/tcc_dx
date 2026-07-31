// 暗黙の基底 ctor: 派生 ctor が mem-init リストを一切書かなくても、
// 各基底のデフォルト ctor が宣言順に呼ばれること（多重継承）。
// MI Phase 1 では明示 `: A(), B()` を書いた基底しか構築されなかった。
class A {
public:
    int a;
    A() { a = 7; }
};

class B {
public:
    int b, c;
    B() { b = 30; c = 500; }        // 非先頭基底: this 調整が必要.
};

class D : public A, public B {
public:
    int d;
    D() { d = 1000; }               // mem-init リスト無し.
};

int main()
{
    D o;
    // a=7 b=30 c=500 d=1000 -> 1537.
    return o.a + o.b + o.c + o.d - 1537;
}
