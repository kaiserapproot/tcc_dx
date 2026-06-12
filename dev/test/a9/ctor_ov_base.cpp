// ctor overload in base mem-initializer: `: B(2.5)` must pick B(double).
class B {
public:
    int v;
    B(int a) { v = a; }
    B(double d) { v = (int)d + 100; }
};

class D : public B {
public:
    int w;
    D(int x) : B(2.5) { w = x; }
};

class E : public B {
public:
    int w;
    E(int x) : B(x + 1) { w = x; }
};

int main() {
    D d(4);
    E e(5);
    return d.v + d.w + e.v + e.w - 117;
}
