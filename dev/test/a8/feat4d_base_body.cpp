class Base {
public:
    int v;
    Base(int x) { v = x; }
};

class Derived : public Base {
public:
    int m;
    Derived(int b, int d) : Base(b) { m = d; }
};

int main() {
    Derived d(10, 20);
    return d.v + d.m - 30;
}
