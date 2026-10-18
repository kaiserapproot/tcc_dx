// NEGATIVE test: must FAIL to compile.
// E has two A subobjects, so E * does not convert to A * ([conv.ptr]), and
// cannot initialize a reference to A * either.
struct A { int x; };
struct B { int y; };
struct D : A, B { int z; };
struct E : D, A { int w; };
typedef A *AP;

int main()
{
    E e;
    E *pe = &e;
    const AP &r = pe;   // expected error: ambiguous base A
    return r != 0;
}
