struct M {
    static int count;
    M() { ++count; }
};
int M::count;
struct X {
    M m[2];
    X() {}
};
int main(void) { X x; return M::count == 2 ? 0 : 1; }
