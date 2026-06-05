class Point {
public:
    int x, y;
};

int main() {
    int Point::*px = &Point::x;
    Point p;
    p.*px = 42;
    Point *pp = &p;
    pp->*px = 99;
    return p.x - 99;
}
