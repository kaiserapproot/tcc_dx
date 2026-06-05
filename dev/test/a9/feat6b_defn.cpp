class C {
public:
    int x;
    int get() const;
};

int C::get() const {
    return x;
}

int main() {
    C c;
    c.x = 2;
    return c.get() - 2;
}
