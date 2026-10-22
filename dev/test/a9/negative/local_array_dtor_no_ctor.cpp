/* The elements of a local class array are not destroyed at scope exit, so
   such an array is rejected.  A class with a destructor but no constructor
   needs no construction code, and its arrays used to compile with the
   element destructors silently skipped; also when the destructor is only
   implicit (a member has one). */
struct M { ~M() { } };
struct A { int v; ~A() { } };
struct H { M m; };

int main()
{
    H h[2];
    A la[3];
    la[0].v = 0;
    (void)h;
    return la[0].v;
}
