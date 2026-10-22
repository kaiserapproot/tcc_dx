/* Same as global_dtor_only_member_no_default_ctor.cpp, with the
   destructor coming from a base that has no default constructor. */
struct B { int v; B(int x) { v = x; } ~B() { } };
struct A : B { };

A g;

int main()
{
    return 0;
}
