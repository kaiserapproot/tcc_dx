struct M { M(); };
M::M() {}
struct A { M m; };
struct B { A a; };

B b;

int main() { return 0; }
