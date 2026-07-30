class Vec2 {
public:
    int x, y;
    int operator+(const Vec2& o);
};

int Vec2::operator+(const Vec2& o) {
    return x + y + o.x + o.y;
}

int main() {
    Vec2 a, b;
    a.x = 1; a.y = 2;
    b.x = 3; b.y = 4;
    return a + b - 10;
}
