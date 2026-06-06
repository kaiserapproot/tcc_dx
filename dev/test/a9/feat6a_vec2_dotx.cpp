class Vec2 {
public:
    int x, y;
    Vec2 copy();
};

Vec2 Vec2::copy() {
    Vec2 r;
    r.x = 10;
    r.y = 20;
    return r;
}

int main() {
    Vec2 a;
    a.x = 1; a.y = 2;
    return a.copy().x - 10;
}
