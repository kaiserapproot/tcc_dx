// An implicit derived constructor must not accept a non-trivial base
// until the compiler can emit the base's default constructor call.
int calls;

struct B {
    B() { calls = calls + 1; }
};

struct D : B {
};

int main()
{
    D d;
    return calls == 1 ? 0 : 1;
}
