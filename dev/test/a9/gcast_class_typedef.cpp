// G-CAST: the SimpleString.cpp:33 shape - a class-scope typedef used
// as a functional cast in the out-of-class static member initializer,
// plus the same cast inside a member body and at a call site.
class S {
public:
    typedef unsigned int size_type;
    static const size_type npos;
    size_type all() const;
};
const S::size_type S::npos = size_type(-1);
S::size_type S::all() const
{
    return size_type(-1);
}
int main()
{
    S s;
    if (S::npos != 0xffffffffu)
        return 1;
    if (s.all() != 0xffffffffu)
        return 2;
    if (S::size_type(-1) != S::npos)
        return 3;
    return 0;
}
