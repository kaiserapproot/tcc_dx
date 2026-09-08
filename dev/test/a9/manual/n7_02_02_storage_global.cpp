static int glog[8];
static int gn;

struct B {
    B()  { glog[gn++] = 1; }
    ~B() { glog[gn++] = 4; }
};

struct M {
    M()  { glog[gn++] = 2; }
    ~M() { glog[gn++] = 3; }
};

struct D : B {
    M m;
};

D g;

int main()
{
    if (gn != 2 || glog[0] != 1 || glog[1] != 2)
        return 1;
    return 0;
}
