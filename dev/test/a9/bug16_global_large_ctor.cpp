// BUG-16 の横断確認: FEAT-4G のグローバル自動 ctor 経路でも、8 バイト超の
// オブジェクトに対する ctor の書き込みが実体へ反映されること。
// cpp_emit_global_ctor_call も gaddrof() のみで this を作っており、
// cpp_emit_base_ctor_call と同型のコピー渡しになる疑いがあった。
class Big {
public:
    int p, q, r, s;                 // 16 バイト.
    Big() { p = 1; q = 2; r = 4; s = 8; }
};

class BigArg {
public:
    int t, u, v;                    // 12 バイト.
    BigArg(int x) { t = x; u = x * 2; v = x * 3; }
};

Big g_default;                      // 引数なしグローバル ctor.
BigArg g_arg(3);                    // 引数ありグローバル ctor.

int main()
{
    if (g_default.p + g_default.q + g_default.r + g_default.s != 15)
        return 1;
    if (g_arg.t + g_arg.u + g_arg.v != 18)
        return 2;
    return 0;
}
