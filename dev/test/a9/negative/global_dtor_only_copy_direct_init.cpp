/* The copy form `A g2(g1);` of the same class is valid C++, but the
   implicit copy constructor is not run on this path: g2 stayed zero and
   was destroyed at exit.  It is rejected until it is implemented (the
   copy-initialization `A g2 = g1;` takes another path and copies). */
struct A {
    int value;
    ~A() {}
};

A g1 = { 7 };
A g2(g1);

int main()
{
    return g2.value;
}
