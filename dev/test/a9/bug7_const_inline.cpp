// BUG-7 regression: const/non-const overload with inline bodies.
// `const Foo cf` must keep VT_CONSTANT (parse_btype class-name path)
// and each call must resolve to the matching const-ness.
class Foo {
public:
    int x;
    int get() { return x; }
    int get() const { return x + 100; }
};

int main() {
    Foo f;
    f.x = 3;
    const Foo cf = f;
    return f.get() + cf.get() - 106;
}
