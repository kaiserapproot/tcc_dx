// MI Phase 1: derived-to-base upcast adjusts the pointer/reference to the
// base subobject.  For the NON-first base the offset is nonzero; the fix
// spans explicit `(B*)`, implicit `B* = &d`, reference parameters, and local
// references.  The first base stays at offset 0 (plain reinterpret).
class A {
public:
    int a;
    int getA() { return a; }
};
class B {
public:
    int b;
    int getB() { return b; }
};
class D : public A, public B {
public:
    int d;
};

int useB(B &r) { return r.getB(); }      // reference-parameter upcast

int main()
{
    D o;
    o.a = 1; o.b = 2; o.d = 3;
    int s = 0;

    B *pe = (B *)&o;  s += pe->getB();    // explicit (B*)  -> 2
    B *pi = &o;       s += pi->getB();    // implicit B* = &o -> 2
    A *pa = &o;       s += pa->getA();    // first base (offset 0) -> 1
    B &r  = o;        s += r.b;           // local reference -> 2
    s += useB(o);                         // reference parameter -> 2

    return s - (2 + 2 + 1 + 2 + 2);       // 9
}
