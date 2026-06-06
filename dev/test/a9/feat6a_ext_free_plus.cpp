class Vec2 {
public:
    int x, y;
};

Vec2 operator+(const Vec2& a, const Vec2& b) {
    Vec2 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    return r;
}

int main() {
    Vec2 a, b;
    a.x = 1; a.y = 2;
    b.x = 3; b.y = 4;
    return (a + b).x + (a + b).y - 10;
}
