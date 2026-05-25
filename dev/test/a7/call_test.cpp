class Point {
public:
    void set(int a, int b);
    int x;
};
void Point::set(int a, int b) { (void)a; }
int main() {
    Point p;
    p.set(1, 2);
    return 0;
}
