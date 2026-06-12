// Base mem-initializer arg conversion: double literal into B(int) param
// must be converted (previously passed raw bits).
class B {
public:
    int v;
    B(int a) { v = a; }
};

class D : public B {
public:
    int w;
    D(int x) : B(2.9) { w = x; }
};

int main() {
    D d(4);
    return d.v + d.w - 6;
}
