struct M {
    static int live;
    M() { ++live; }
    ~M() { --live; }
};
int M::live;
struct X { M m[3]; };
int main(void) { X x; return 0; }
