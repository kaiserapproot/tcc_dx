class Vec2 {
public:
    int x, y;
    Vec2 operator+(Vec2 o);
};

Vec2 Vec2::operator+(Vec2 o) {
    Vec2 r;
    r.x = x + o.x;
    r.y = y + o.y;
    return r;
}

int main() {
    Vec2 a, b, c;
    a.x = 1; a.y = 2;
    b.x = 3; b.y = 4;
    c = a + b;
    return c.x + c.y - 10;
}
