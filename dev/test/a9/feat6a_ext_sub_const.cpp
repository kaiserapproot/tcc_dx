class Vec2 {
public:
    int x, y;
    const int& operator[](int i) const;
};

const int& Vec2::operator[](int i) const {
    return i ? y : x;
}

int main() {
    Vec2 v;
    v.x = 5;
    v.y = 7;
    const Vec2 cv = v;
    return cv.x + cv.y - 12;
}
