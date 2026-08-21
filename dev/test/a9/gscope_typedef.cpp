// G1: leading :: in typedef position (the cuconfig.h:178 form).
class C {
public:
    int v;
};
typedef ::C D;
int main()
{
    D d;
    d.v = 5;
    return d.v - 5;
}
