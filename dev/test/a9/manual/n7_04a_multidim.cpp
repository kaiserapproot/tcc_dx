struct M { M() {} };
struct X { M m[2][3]; };
int main(void) { X x; return 0; }
