// G-CAST negative: T(a, b) must be refused rather than silently taken
// as a comma expression.
typedef unsigned int size_type;
int main()
{
    size_type a;
    a = size_type(1, 2);
    return (int)a;
}
