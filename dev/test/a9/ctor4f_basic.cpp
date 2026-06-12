// FEAT-4F: `P a;` calls the user default ctor implicitly.
class P {
public:
    int v;
    P() { v = 7; }
};

int main() {
    P a;
    return a.v - 7;
}
