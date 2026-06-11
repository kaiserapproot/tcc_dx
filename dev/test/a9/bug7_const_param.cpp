// BUG-7 regression: leading `const Foo` in declarations keeps the
// qualifier without breaking const member calls through pointers.
class Foo {
public:
    int x;
    int get() const;
    int get();
};

int Foo::get() const { return x + 10; }
int Foo::get() { return x; }

int main() {
    Foo f;
    f.x = 7;
    const Foo c = f;
    const Foo *p = &c;
    return f.get() + p->get() - 24;
}
