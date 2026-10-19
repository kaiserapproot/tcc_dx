/* C++ shared library without destructor runtime: class + ctor only */
struct Acc {
    int base;
    Acc(int b) : base(b) {}
    int add(int x) { return base + x; }
};
Acc g_acc(30);
extern "C" int so_cpp_get(int x) { return g_acc.add(x); }
