/* `d.Derived::x` where two bases declare x (one non-static, one
   static): the lookup in Derived's scope is ambiguous (C++98 10.2/2);
   g++ rejects it.  The non-static base comes first on purpose. */
struct ObjectBase { int x; };
struct StaticBase { static int x; };
int StaticBase::x = 11;
struct Derived : ObjectBase, StaticBase { };
int main() { Derived d; return d.Derived::x; }
