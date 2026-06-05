/* compile-only: must fail — const object calling non-const method */
class C {
public:
    int x;
    int get() { return x; }
};

int main() {
    C t;
    const C c = t;
    return c.get();
}
