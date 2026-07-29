// Multiple inheritance (non-virtual), Phase 1: two base subobjects laid out
// in declaration order (A at 0, B after A), data-member access from both
// bases, and member calls - including a method inherited from the NON-first
// base, whose `this` must be adjusted to the base subobject.
class A {
public:
    int a;
    int getA() { return a; }
    void setA(int x) { a = x; }
};
class B {
public:
    int b;
    int getB() { return b; }             // B is the non-first base
    void setB(int x) { b = x; }
};
class D : public A, public B {
public:
    int d;
};

int main()
{
    D o;
    o.a = 10; o.b = 20; o.d = 30;        // data members of both bases + own
    if (o.getA() != 10) return 1;        // first base method  (this + 0)
    if (o.getB() != 20) return 2;        // second base method (this + off(B))
    o.setA(11);
    o.setB(22);                          // write through non-first base method
    if (o.a != 11 || o.b != 22) return 3;
    // layout: A(4) + B(4) + d(4) = 12
    if (sizeof(D) != 12) return 4;
    return 0;
}
