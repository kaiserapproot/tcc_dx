class Calc {
public:
    int get() { return 42; }
};
int main() {
    Calc c;
    return c.get() == 42 ? 0 : 1;
}