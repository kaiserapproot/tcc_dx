struct A {
    A(int x = 0);
};
A::A(int x) { (void)x; }
int main() { A a; return 0; }
