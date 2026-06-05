class C {
public:
    int x;
    int get() const { return x; }
};

int main() {
    C t;
    t.x = 4;
    const C c = t;
    return c.get() - 4;
}
