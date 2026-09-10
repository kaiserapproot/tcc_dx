// N7-00 storage THREAD_LOCAL: no viable default ctor (N6 TLS rules may apply).
struct P {
    P(int x) { (void)x; }
};
int main(void)
{
    thread_local P tls_a;
    (void)tls_a;
    return 0;
}
