class Point {
    int x;
public:
    void set(int a, int b);
};
void Point::set(int a, int b) { (void)a; (void)b; }
int main() { Point p; p.set(1,2); return 0; }
