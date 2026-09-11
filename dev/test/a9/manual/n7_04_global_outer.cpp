struct M {
    static int count;
    M() { ++count; }
};
int M::count;
struct X { M m[2]; };
static X g_x;
int main(void) { return M::count == 2 ? 0 : 1; }
