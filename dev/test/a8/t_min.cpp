class Base { public: int x; };
class Derived : public Base { public: int y; };
int main() { Derived d; d.x = 1; return d.x; }
