// An explicit outer ctor must reject a base without a default constructor.
struct B { B(int value) { } };
struct D : B { D() { } };
int main() { D d; return 0; }
