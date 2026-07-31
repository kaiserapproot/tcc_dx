// BUG-16 横断: 8 バイト超オブジェクトのローカル自動 dtor で、`this` が
// 実体のアドレスであること（コピーのアドレスでないこと）。
// 修正前は Win64 ABI が by-value 構造体としてコピーを staging するため、
// dtor が `this` を外部へ公開すると dangling な一時領域を指していた。
class Big {
public:
    int a, b, c, d;                 // 16 バイト.
    Big() { a = 1; b = 2; c = 3; d = 4; }
    ~Big();
};

Big *g_seen = 0;

Big::~Big() { g_seen = this; }

int main()
{
    Big *addr;
    {
        Big o;
        addr = &o;
        if (o.a + o.b + o.c + o.d != 10)
            return 1;
    }                               // ここで ~Big が走る.
    if (g_seen != addr)
        return 2;
    return 0;
}
