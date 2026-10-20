/* Covariant return through a non-primary base would need a thunk that
   adjusts the returned pointer; tpp has none, so it is rejected instead
   of returning a pointer to the wrong subobject. */
struct X { int x; };
struct Y { int y; };
struct Z : X, Y { };
struct B { virtual Y *get() { return 0; } };
struct D : B { Z z; Z *get() { return &z; } };
int main() { D d; return d.get() != 0 ? 0 : 1; }
