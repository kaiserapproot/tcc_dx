// G1: leading :: in a declaration's type position, global and local.
class C {
public:
    int v;
};
::C g;
int main()
{
    ::C d;
    d.v = 2;
    g.v = 3;
    return d.v + g.v - 5;
}
