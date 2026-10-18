// N6-07-04 gate H: thread_local and a call to a function nobody defines, so
// tcc_relocate() fails after it has allocated run_ptr.  tcc_delete() must not
// then call the N6 delete gate, whose code was never made executable.
extern "C" int n6_07_04_function_nobody_defines(void);

static volatile int g_tls_ctor;

struct TlsObj {
    TlsObj() { ++g_tls_ctor; }
    ~TlsObj() { --g_tls_ctor; }
};
thread_local TlsObj tls;

extern "C" int touch_tls(void)
{
    (void)&tls;
    return n6_07_04_function_nobody_defines();
}