// G-CAST: functional cast on a file-scope typedef, and a same-TU
// ordinary call that must stay an ordinary call.
typedef unsigned int size_type;
int f(int x)
{
    return x + 1;
}
int main()
{
    size_type a;
    size_type b;
    a = size_type(-1);
    b = (size_type)-1;
    if (a != b)
        return 1;
    if (a != 0xffffffffu)
        return 2;
    if (f(-1) != 0)
        return 3;
    if (int('A') != 65)
        return 4;
    return 0;
}
