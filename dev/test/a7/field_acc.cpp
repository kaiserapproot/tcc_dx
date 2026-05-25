class Point { public: int x; void set(int a); };
void Point::set(int a) { (void)a; }
int main() { Point p; p.x = 1; return 0; }
