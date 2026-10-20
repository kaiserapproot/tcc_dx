/* A protected base of a class the overrider does not derive from is not
   accessible from the overrider (C++98 11.2), so the covariant return is
   invalid.  g++: "invalid covariant return type". */
struct A { int a; };
struct B { virtual A *f() { return 0; } };
struct C : protected A { };
struct D : B { C c; C *f() { return &c; } };
int main() { D d; return d.f() != 0 ? 0 : 1; }
