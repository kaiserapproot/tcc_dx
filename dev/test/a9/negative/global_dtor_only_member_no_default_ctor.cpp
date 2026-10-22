/* A class with no user constructor whose member has no default
   constructor cannot be default-initialized, as for a local `A a;`.  Its
   global objects were registered only because the class needs
   destruction: the program compiled and the member destructor ran at exit
   on a member that was never constructed. */
struct M { int v; M(int x) { v = x; } ~M() { } };
struct A { M m; };

A g;

int main()
{
    return 0;
}
