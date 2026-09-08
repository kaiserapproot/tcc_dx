struct A {
    A();
    A(int);
};
A::A() {}
A::A(int x) { (void)x; }
int main() { A a; return 0; }
