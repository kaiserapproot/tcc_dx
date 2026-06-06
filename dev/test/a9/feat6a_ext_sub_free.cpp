class Vec2 {
public:
    int x, y;
};

/* TCC extension: non-member operator[] (ISO C++98 requires member). */
int& operator[](Vec2& v, int i) {
    if (i)
        return v.y;
    return v.x;
}

int main() {
    Vec2 v;
    v.x = 5;
    v.y = 7;
    v[1] = 3;
    return v.x + v.y - 8;
}
