class Vec2 {
    int x, y;
public:
    void set(int a, int b);
    int sum();
};

void Vec2::set(int a, int b) {
    x = a;
    y = b;
}

int Vec2::sum() {
    return x + y;
}

int main() {
    Vec2 a;
    a.set(1, 2);
    return a.sum() - 3;
}
