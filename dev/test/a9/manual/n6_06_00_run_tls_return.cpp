// N6-06-00 measurement: -run return with thread_local (compile/link probe).
struct T {
    int tag;
    T() { tag = 1; }
    ~T() { tag = 0; }
};
thread_local T tls;
int main() {
    (void)tls.tag;
    return 7;
}
