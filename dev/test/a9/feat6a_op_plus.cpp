class Vec2 {
public:
    int x, y;
    int operator+(Vec2 o);
};

int Vec2::operator+(Vec2 o) {
    return x + o.x + o.y;
}

int main() {
    Vec2 a, b;
    a.x = 10; a.y = 0;
    b.x = 3; b.y = 4;
    return a + b - 17;
}
