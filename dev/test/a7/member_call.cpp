class Point {
    int x;
    int y;
public:
    void set(int a, int b);
    int getX();
};

void Point::set(int a, int b) {
    x = a;
    y = b;
}

int Point::getX() {
    return x;
}

int main() {
    Point p;
    p.set(3, 4);
    if (p.getX() != 3)
        return 1;
    return 0;
}
