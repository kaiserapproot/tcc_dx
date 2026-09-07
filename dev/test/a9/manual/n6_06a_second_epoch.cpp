#line 1 "n6_06a_second.cpp"
static volatile int g_ctor;
static volatile int g_dtor;
struct T { T(){ ++g_ctor; } ~T(){ ++g_dtor; } };
thread_local T tls;
extern "C" unsigned __cdecl __tcc_cpp_tls_n6_run_epoch(void);
int main(void) {
    (void)&tls;
    return (int)(__tcc_cpp_tls_n6_run_epoch() * 100u + (unsigned)g_ctor * 10u);
}
