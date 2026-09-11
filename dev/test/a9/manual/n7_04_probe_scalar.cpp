struct M {
    static int count;
    M() { ++count; }
};
int M::count;
struct X {
    M m;
    X() {}
};
int main(void) { X x; return M::count == 1 ? 0 : 1; }
