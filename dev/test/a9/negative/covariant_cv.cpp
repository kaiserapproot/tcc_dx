/* A covariant return must not be more cv-qualified than the overridden
   function's (C++98 10.3/5).  g++: "invalid covariant return type". */
struct B { virtual B *f() { return this; } };
struct D : B { const D *f() { return this; } };
int main() { D d; return d.f() != 0 ? 0 : 1; }
