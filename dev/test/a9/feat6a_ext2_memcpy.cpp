// FEAT-6A-ext2 P1 regression: class WITHOUT operator= keeps memcpy copy
class Plain {
public:
    int a;
    int b;
};

int main() {
    Plain p, q;
    p.a = 3;
    p.b = 4;
    q.a = 0;
    q.b = 0;
    q = p;              /* default copy (memcpy path) */
    return q.a + q.b - 7;
}
