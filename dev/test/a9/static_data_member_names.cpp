/* Static data members are distinct objects per class, and neither a local
   nor a global of the same name may take their place (C++98 9.4.2, 3.4.1).
   Expected values are g++ -std=c++98 results.  Exit code = number of the
   first failing check. */
struct A { static int x; int y; A() : y(1) {} int get() { return x; } int lget() { int x = 4; return x; } };
int A::x = 17;
struct C { static int x; int get() const { return x + 1; } };
int C::x = 23;
int x = 50;                                 /* a global of the same name */

struct B : A { int bget() { return x * 10; } };   /* inherited static */

struct K { static const int N; static int arr[3];
           static int sget() { return x + N; } };   /* static member fn body */
const int K::N = 3;
int K::arr[3] = { 1, 2, 3 };
struct K2 { static int x; };
int K2::x = 1;

int main()
{
    A a; C c; B b;
    {
        int x = 99;                         /* a local of the same name */
        if (a.A::x != 17 || a.x != 17 || A::x != 17) return 1;
        if (c.x != 23 || C::x != 23) return 2;
        if (x != 99) return 3;
        a.x = 5;
        if (A::x != 5 || C::x != 23 || x != 99) return 4;
    }
    if (::x != 50 || x != 50) return 5;
    if (a.get() != 5 || c.get() != 24 || a.lget() != 4) return 6;
    if (b.bget() != 50 || b.B::x != 5 || B::x != 5) return 7;
    A::x = 6;
    if (b.get() != 6 || ::x != 50) return 8;
    if (K::N != 3 || K::arr[2] != 3) return 9;
    if (K::sget() != 50 + 3) return 10;     /* K has no x: the global */
    if (K2::x != 1) return 11;
    return 0;
}
