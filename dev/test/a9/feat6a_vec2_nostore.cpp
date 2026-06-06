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
    Vec2 a;
    a.x = 1; a.y = 2;
    return a.copy().x + a.copy().y - 3;
}
