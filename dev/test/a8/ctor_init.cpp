class Base { public: int v; };
class Derived : public Base {
public:
    int m;
};
void init_derived() {
    Derived d;
    d.m = 0;
    d.v = 0;
}
int main() { init_derived(); return 0; }
