class Base { public: int get(); };
int Base::get() { return 42; }
class Derived : public Base { public: int y; };
int main() { Derived d; return d.get() - 42; }
