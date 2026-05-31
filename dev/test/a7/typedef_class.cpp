class Point {
public:
    int x;
};

typedef Point P;

int main() {
    P p;
    p.x = 1;
    return p.x - 1;
}
