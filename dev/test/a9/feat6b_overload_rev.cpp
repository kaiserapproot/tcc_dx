class Foo {
public:
    int x;
    int get() const;
    int get();
};

int Foo::get() const {
    return x + 100;
}

int Foo::get() {
    return x;
}

int main() {
    Foo f;
    f.x = 3;
    const Foo cf = f;
    return f.get() + cf.get() - 106;
}
