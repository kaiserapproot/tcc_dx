/* obj.X::m with X not a base of obj's class (C++98 5.2.5/4): g++ rejects
   it ("'X' is not a base of 'D'"). */
struct X { int get() { return 1; } };
struct B { int get() { return 2; } };
struct D : B { };
int main() { D d; return d.X::get(); }
