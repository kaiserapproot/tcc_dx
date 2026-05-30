class Point {
    int x;
public:
    void set(int a, int b);
    int add(int d);
};

void Point::set(int a, int b) { x = a; }
int Point::add(int d) { return x + d; }

int main() {
    Point p;
    p.set(3, 0);
    return p.add(5) == 8 ? 0 : 5;
}