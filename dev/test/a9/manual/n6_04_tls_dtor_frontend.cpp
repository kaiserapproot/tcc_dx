// N6-04 frontend acceptance (NONTRIVIAL_DTOR_FRONTEND_ACCEPTANCE=YES).
// N6-02 では "thread_local object with non-trivial destructor is unsupported in
// N6-02" で fail-closed だった形 (旧 n6_02_tls_dtor_unsupported.cpp) を, N6-04 で
// 初めて受理する.  ここでは compile が通り, main thread から touch しても正常終了
// することだけを見る.  main thread の dtor 実行は N6-05 (MAIN_THREAD_TLS_DTOR=
// DEFERRED_TO_N6_05) なので, dtor が走らないことを exit code に反映させる:
//   value = 1 (ctor) -> main return 時点で dtor はまだ走らない -> 0 を返す.
// worker thread 側の dtor 実行は n6_04_tls_dtor_single.cpp 等で測る.
struct P {
    int value;
    P();
    ~P();
};

thread_local P object;

P::P() { value = 1; }
P::~P() { value = 0; }

int main()
{
    // touch: main thread で construct (value=1). 戻り値 0 = 正常.
    return object.value == 1 ? 0 : 1;
}
