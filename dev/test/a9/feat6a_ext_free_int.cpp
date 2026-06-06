class A {
public:
    int v;
};

int operator+(A a, A b) {
    return a.v + b.v;
}

int main() {
    A a, b;
    a.v = 3;
    b.v = 7;
    return a + b - 10;
}
