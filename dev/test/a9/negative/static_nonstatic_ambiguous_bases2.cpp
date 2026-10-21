/* (bases in the other order) `Derived::x` where one base declares a static x and another a
   non-static x: the name lookup is ambiguous whether or not the members
   are static (C++98 10.2/2); g++ and clang reject it. */
struct StaticBase { static int x; };
int StaticBase::x = 11;
struct ObjectBase { int x; };
struct Derived : ObjectBase, StaticBase {
    int get() { return Derived::x; }
};
int main() { Derived d; return d.get(); }
