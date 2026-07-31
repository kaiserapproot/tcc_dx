// 単一継承の回帰: 暗黙基底 ctor / 自動基底 dtor が単一継承でも動くこと。
// 併せて 8 バイト超の基底（BUG-16 経路）でも暗黙 ctor の書き込みが
// 反映されることを確認する。
// 注記: 派生クラス自身が ctor / dtor を宣言していることが前提（本実装の
// スコープ）。派生に dtor が無い場合の基底 dtor 自動合成は未対応。
int dtor_log = 0;

class Base {
public:
    int p, q, r;                    // 12 バイト: レジスタ渡し不可.
    Base() { p = 2; q = 4; r = 8; }
    ~Base() { dtor_log = dtor_log + 10; }
};

class Derived : public Base {
public:
    int s;
    Derived() { s = 16; }           // mem-init 無し -> Base() が暗黙に走る.
    ~Derived() { dtor_log = dtor_log + 1; }
};

int main()
{
    {
        Derived o;
        if (o.p + o.q + o.r + o.s != 30)
            return 1;
    }
    // ~Derived (+1) の後に ~Base (+10) -> 11.
    if (dtor_log != 11)
        return 2;
    return 0;
}
