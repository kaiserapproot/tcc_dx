class Point {
public:
    int x, y;
};

int main() {
    int Point::*px = &Point::x;
    int Point::*py = &Point::y;
    Point p;
    p.x = 1;
    p.y = 2;
    return (p.*px + p.*py) - 3;
}
