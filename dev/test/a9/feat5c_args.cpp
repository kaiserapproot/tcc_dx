// FEAT-5C: virtual PMF that takes an argument and returns a value, invoked
// polymorphically through a base pointer.
class Shape {
public:
    Shape() {}
    virtual int area(int k) { return k; }        // base: linear
};
class Square : public Shape {
public:
    Square() {}
    virtual int area(int k) { return k * k; }    // override: k^2
};

int main()
{
    int (Shape::*pmf)(int) = &Shape::area;
    Square s;
    Shape *p = (Shape *)&s;
    int rp = (p->*pmf)(5);       // Square::area(5) = 25
    Shape base;
    int rb = (base.*pmf)(5);     // Shape::area(5)  = 5
    return rp * 100 + rb - 2505; // 25*100 + 5 - 2505 = 0
}
