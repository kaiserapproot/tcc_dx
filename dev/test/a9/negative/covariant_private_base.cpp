/* The class of a covariant return must be an accessible base
   (C++98 10.3/5).  g++: "invalid covariant return type". */
struct A { int a; };
struct B { virtual A *f() { return 0; } };
struct C : private A { };
struct D : B { C c; C *f() { return &c; } };
int main() { D d; return d.f() != 0 ? 0 : 1; }
