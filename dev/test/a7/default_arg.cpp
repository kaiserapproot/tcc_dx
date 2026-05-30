class Foo {
public:
    int val;
    void set(int x = 10);
};

void Foo::set(int x) {
    val = x;
}

int main() {
    Foo f;
    f.set();
    return f.val == 10 ? 0 : 1;
}
