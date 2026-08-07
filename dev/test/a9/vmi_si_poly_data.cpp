// BUG-17 regression: SINGLE inheritance from a polymorphic base that has
// data members.  The old layout inserted a second vptr into the derived
// class, shifting the base subobject by 8 bytes, so base-pointer data
// access and non-virtual base methods silently read the wrong slot (the
// never-initialized inner vptr).  The shared-primary-vptr layout keeps the
// base subobject at offset 0 and one single vptr.
class B {
public:
    int x;
    virtual int f() { return 1; }
    int getx() { return x; }         // non-virtual base method
};
class D : public B {
public:
    int y;
    virtual int f() { return 2; }
};

typedef char szD[(sizeof(D) == 24) ? 1 : -1];  // [vptr][x][y]+pad: ONE vptr

int main()
{
    D o;
    B *pb;
    o.x = 10; o.y = 20;
    pb = (B *)&o;
    if (pb->f() != 2) return 1;      // virtual dispatch (worked before too)
    if (pb->x != 10) return 2;       // data through B* (was garbage)
    if (pb->getx() != 10) return 3;  // non-virtual base method (was garbage)
    if (o.f() != 2) return 4;
    if (o.getx() != 10) return 5;
    return 0;
}
