int calls;
struct B { B(int value = 9) { calls = value; } };
struct D : B { D() { } };
int main() { calls = 0; D d; return calls == 9 ? 0 : 1; }
