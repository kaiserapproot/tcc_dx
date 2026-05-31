class Pt {
public:
    int x, y;
    Pt(int a, int b);
};

Pt::Pt(int a, int b) : x(a), y(b) {}

int main() {
    Pt p(3, 4);
    return p.x + p.y - 7;
}
