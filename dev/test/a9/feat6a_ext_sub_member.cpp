class Vec2 {
public:
    int x, y;
    int& operator[](int i);
};

int& Vec2::operator[](int i) {
    return i ? y : x;
}

int main() {
    Vec2 v;
    v.x = 5;
    v.y = 7;
    v[1] = 3;
    return v.x + v.y - 8;
}
