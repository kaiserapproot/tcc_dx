// N6-05A measurement only: return from main with thread_local (TLS runtime).
struct T { int v; T() { v = 1; } ~T() { v = 0; } };
thread_local T tls;
int main() { (void)tls.v; return 7; }
