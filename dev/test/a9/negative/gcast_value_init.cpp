// G-CAST negative: T() value-initialization is out of scope - silently
// yielding an uninitialized value would be a miscompile.
typedef unsigned int size_type;
int main()
{
    size_type a;
    a = size_type();
    return (int)a;
}
