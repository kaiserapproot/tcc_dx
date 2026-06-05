class Foo {
public:
    int x;
    int get();
    int get() const;
};

int Foo::get() {
    return x;
}

int Foo::get() const {
    return x + 100;
}

int main() {
    Foo f;
    f.x = 3;
    const Foo cf = f;
    return f.get() + cf.get() - 106;
}
