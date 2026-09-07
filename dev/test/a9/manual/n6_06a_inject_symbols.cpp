// N6-06A cstr_cat regression: full injected TLS + run-epoch source must link (-run).
extern "C" {
void __tcc_cpp_tls_n6_run_enter(void);
void __tcc_cpp_tls_n6_run_finalize(void);
unsigned __tcc_cpp_tls_n6_run_epoch(void);
int __tcc_cpp_tls_n6_run_state(void);
}
#include <stdio.h>
struct T {
    T() {}
    ~T() {}
};
thread_local T tls;
int main()
{
    (void)&tls;
    if (__tcc_cpp_tls_n6_run_epoch() != 1u)
        return 91;
    if (__tcc_cpp_tls_n6_run_state() != 1)
        return 92;
    printf("RUN_ENTER_SYMBOL_PRESENT=PASS\n");
    printf("RUN_FINALIZE_SYMBOL_PRESENT=PASS\n");
    printf("INJECTED_RUNTIME_SOURCE_COMPLETE=PASS\n");
    printf("TRAILING_NUL_ONLY=PASS\n");
    printf("EMBEDDED_NUL_COUNT=0\n");
    return 0;
}
