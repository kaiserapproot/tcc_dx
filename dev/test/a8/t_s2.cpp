class Foo { public: static int bar(); };
int Foo::bar() { return 10; }
int main() { return Foo::bar(); }