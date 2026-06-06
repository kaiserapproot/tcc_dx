class Vec2 {
public:
    int x, y;
};

int& operator[](Vec2& v, int i) {
    return i ? v.y : v.x;
}

int main() {
    Vec2 v;
    v.x = 5;
    v.y = 7;
    v[1] = 3;
    return v.x + v.y - 8;
}
