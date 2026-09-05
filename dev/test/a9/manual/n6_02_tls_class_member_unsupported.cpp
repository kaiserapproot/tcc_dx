// N6-02 REVIEW FIX-5 fail-closed: non-static `thread_local` data member.
// 11f4831 では VT_CPP_TLS が field type に残り, 後段の汎用診断
// ("'value' に対する無効な型") で失敗していた (実測, silent ではない).
// 未対応形式を名指しする診断に置き換える.
// 期待: "thread_local class member is unsupported in N6-02"
struct Holder {
    thread_local int value;
};

int main()
{
    Holder h;
    return h.value;
}
