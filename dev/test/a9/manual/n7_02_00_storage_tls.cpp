struct M {
    static int count;
    M(void) { ++count; }
};

int M::count;

struct A {
    M m;
};

thread_local A tls_a;

int main(void)
{
    M::count = 0;
    (void)&tls_a;
    return M::count == 1 ? 0 : 1;
}
