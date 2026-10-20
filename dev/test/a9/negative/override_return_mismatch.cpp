/* An override must return the same type as the virtual function it
   overrides, or a covariant pointer/reference (C++98 10.3/5).  g++ also
   rejects this ("conflicting return type"). */
struct B { virtual int f() { return 1; } };
struct D : B { double f() { return 2.0; } };
int main() { D d; return (int)d.f(); }
