static int g_seq;

class A {
public:
    ~A() { g_seq = g_seq * 10 + 1; }
};

class B {
public:
    ~B() { g_seq = g_seq * 10 + 2; }
};

int main() {
    { B b; A a; }
    return g_seq - 12;
}
