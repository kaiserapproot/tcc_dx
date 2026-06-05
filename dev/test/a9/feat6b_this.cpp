class C {
public:
    int x;
    int get() const { return x; }
};

int main() {
    C c;
    c.x = 7;
    return c.get() - 7;
}
