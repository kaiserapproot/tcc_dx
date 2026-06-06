class Vec2 {
public:
    int x, y;
    int& operator[](int i);
};

int& Vec2::operator[](int i) {
    if (i)
        return y;
    return x;
}

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
    return (a + b)[0] + (a + b)[1] - 10;
}
