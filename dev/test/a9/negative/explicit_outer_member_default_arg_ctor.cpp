// An implicit member ctor with default arguments must not be silently skipped.
int calls;
struct M { M(int value = 7) { calls = value; } };
struct X { M m; X() { } };
int main() { calls = 0; X x; return calls == 7 ? 0 : 1; }
