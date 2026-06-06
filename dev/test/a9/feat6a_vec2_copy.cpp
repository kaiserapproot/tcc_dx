class Vec2 {
public:
    int x, y;
    Vec2 copy();
};

Vec2 Vec2::copy() {
    Vec2 r;
    r.x = x;
    r.y = y;
    return r;
}

int main() {
    Vec2 a, c;
    a.x = 1; a.y = 2;
    c = a.copy();
    return c.x + c.y - 3;
}
