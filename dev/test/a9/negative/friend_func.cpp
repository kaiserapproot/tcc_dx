// G2 negative: a friend FUNCTION declaration also declares that
// function, so skipping it would silently drop the declaration -
// it must be rejected with an explicit error instead.
class C {
    friend int fn(C&);
public:
    int v;
};
int main()
{
    return 0;
}
