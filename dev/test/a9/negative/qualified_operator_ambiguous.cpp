/* `d.D::operator+(1)` where two bases declare operator+: ambiguous
   (C++98 10.2/2); g++: "request for member 'operator+' is ambiguous". */
struct L { int operator+(int) { return 11; } };
struct R { int operator+(int) { return 22; } };
struct D : L, R { };
int main() { D d; return d.D::operator+(1); }
