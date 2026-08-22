// G3 P3: qualified type names Class::typedef in declarations, sizeof,
// globals, and through a derived class (base typedef via D::T).
class S {
public:
    typedef unsigned int size_type;
    typedef char value_type;
};
class D : public S {
};
S::size_type g;
int main()
{
    S::size_type n;
    S::value_type c;
    D::size_type m;
    n = 5;
    c = 'k';
    g = 2;
    m = 1;
    if (sizeof(S::size_type) != sizeof(unsigned int))
        return 1;
    if (sizeof(c) != 1)
        return 2;
    return (int)(n + g + m - 8);
}
