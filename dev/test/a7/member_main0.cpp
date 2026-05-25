class Point {
    int x;
public:
    void set(int a, int b);
    int getX();
};
void Point::set(int a, int b) { x = a; }
int Point::getX() { return x; }
int main() { return 0; }
