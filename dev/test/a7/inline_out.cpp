class Calc {
public:
    int add(int a, int b);
};
int Calc::add(int a, int b) { return a + b; }
int main() {
    Calc c;
    return c.add(3, 4) == 7 ? 0 : 1;
}