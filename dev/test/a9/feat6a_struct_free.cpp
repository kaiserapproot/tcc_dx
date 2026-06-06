struct Vec2 {
    int x, y;
};

Vec2 make() {
    Vec2 r;
    r.x = 1;
    r.y = 2;
    return r;
}

int main() {
    Vec2 v = make();
    return v.x + v.y - 3;
}
