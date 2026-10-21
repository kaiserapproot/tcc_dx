/* A static member named through a base class that occurs twice
   (D -> L -> A, D -> R -> A).  A static member belongs to no subobject,
   so no unique A subobject is needed (C++98 10.2/5, 5.2.5/4).
   GCC 14.2 accepts this (per the PR #52 review); GCC 13.3 rejects it as
   "'A' is an ambiguous base of 'D'".  Exit code = first failing check. */
struct A {
    static int x;
    static int f() { return 7; }
};
int A::x = 2;

struct L : A {};
struct R : A {};
struct D : L, R {};

int main()
{
    D d;
    if (d.A::f() != 7) return 1;
    if (d.A::x != 2) return 2;
    d.A::x = 3;
    if (A::x != 3) return 3;
    return 0;
}
