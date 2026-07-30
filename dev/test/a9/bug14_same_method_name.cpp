// BUG-14: two unrelated classes with a same-named, same-signature method
// used to mangle to the same link name (__tcc_f_), external_sym merged them
// into one Sym, and the two bodies clobbered each other -> tcc crashed at
// run time.  Member function link names are now class-qualified, so each
// stays distinct.  Both inline and out-of-class forms are covered.
class A {
public:
    int f() { return 1; }
    int g(int x);
};
class B {
public:
    int f() { return 2; }
    int g(int x);
};

int A::g(int x) { return x + 10; }   // same name/signature, different class
int B::g(int x) { return x + 20; }

int main()
{
    A a;
    B b;
    // 1*1000 + 2*100 + 11*... keep it simple and exact:
    int r = a.f() * 1000 + b.f() * 100 + a.g(1) + b.g(1);
    return r - (1000 + 200 + 11 + 21);   // 1000+200+11+21 = 1232 -> 0
}
