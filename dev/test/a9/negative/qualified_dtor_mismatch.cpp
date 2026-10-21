/* In obj.B::~X() the name after ~ must denote B (C++98 12.4/12);
   g++ rejects ~D here. */
struct B { ~B() { } };
struct D : B { ~D() { } };
int main() { D d; d.B::~D(); return 0; }
