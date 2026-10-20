/* The qualifier itself names a base that occurs twice: still ambiguous
   (g++: "'A' is an ambiguous base of 'D'"). */
struct A { int value; A() : value(1) {} int get() { return value; } };
struct L : A { };
struct R : A { };
struct D : L, R { };
int main() { D d; return d.A::get(); }
