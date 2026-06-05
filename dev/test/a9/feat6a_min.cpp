class Vec2 {
public:
    int x, y;
    int operator+(Vec2 o);
};

int Vec2::operator+(Vec2 o) {
    return x + o.x;
}

int main() {
    Vec2 a, b;
    a.x = 3; b.x = 7;
    return a + b - 10;
}
