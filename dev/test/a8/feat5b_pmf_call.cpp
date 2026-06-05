class Point {
public:
    int x;
    void set(int v);
    int get();
};

void Point::set(int v) { x = v; }
int Point::get() { return x; }

int main() {
    void (Point::*pset)(int) = &Point::set;
    int (Point::*pget)() = &Point::get;
    Point p;
    (p.*pset)(42);
    return (p.*pget)() - 42;
}
