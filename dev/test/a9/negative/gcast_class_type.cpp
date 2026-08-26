// G-CAST negative: a class-type temporary Foo(1) needs ctor selection
// (G4 territory) and must be refused, not guessed.
class Foo {
public:
    int v;
};
int main()
{
    int n;
    n = Foo(1).v;
    return n;
}
