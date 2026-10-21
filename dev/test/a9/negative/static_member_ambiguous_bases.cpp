/* `D::x` where two different bases each declare a static x: ambiguous
   (C++98 10.2/2); g++ rejects it. */
struct S1 { static int x; };
struct S2 { static int x; };
int S1::x = 1;
int S2::x = 2;
struct D : S1, S2 { };
int main() { return D::x; }
