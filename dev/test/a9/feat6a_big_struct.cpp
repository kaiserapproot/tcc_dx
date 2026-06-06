class Big {
public:
    int a, b, c;
    Big make();
};

Big Big::make() {
    Big r;
    r.a = 1;
    r.b = 2;
    r.c = 3;
    return r;
}

int main() {
    Big x, y;
    x.a = 0; x.b = 0; x.c = 0;
    y = x.make();
    return y.a + y.b + y.c - 6;
}
