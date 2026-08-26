// An implicit base ctor must not skip a nested non-trivial member ctor.
struct N { N() { } };
struct B { N n; };
struct D : B { D() { } };
int main() { D d; return 0; }
