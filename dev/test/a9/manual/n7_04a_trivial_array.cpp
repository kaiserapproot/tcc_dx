struct X { int a[4]; };
int main(void) { X x; x.a[0] = 1; return x.a[0] == 1 ? 0 : 1; }
