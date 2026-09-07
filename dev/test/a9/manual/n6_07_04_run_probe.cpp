// N6-07-04 gate E: tcc_run() TLS finalize then tcc_delete().
struct RunTls {
    RunTls() : v(42) {}
    ~RunTls() {}
    int v;
};
thread_local RunTls tls;

int main(void)
{
    return tls.v;
}
