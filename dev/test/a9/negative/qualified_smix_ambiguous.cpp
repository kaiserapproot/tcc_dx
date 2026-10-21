/* An overload set mixing static and non-static members, named through a
   base that occurs twice: if a non-static one were chosen there would be
   no unique subobject for `this`.  tpp rejects this shape explicitly. */
struct A { static int f(int n) { return n; } int f(double) { return 0; } };
struct L : A { };
struct R : A { };
struct D : L, R { };
int main() { D d; return d.A::f(1); }
