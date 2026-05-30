class Base { public: int get() { return 42; } };
class Derived : public Base {};
int main() { Derived d; return d.get(); }
