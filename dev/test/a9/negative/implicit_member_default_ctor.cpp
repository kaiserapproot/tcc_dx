// An implicit outer constructor must not accept a non-trivial member
// until the compiler can emit the member's default constructor call.
int calls;

struct M {
    M() { calls = calls + 1; }
};

struct X {
    M m;
};

int main()
{
    X x;
    return calls == 1 ? 0 : 1;
}
