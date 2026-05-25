extern "C" {
    int c_add(int a, int b);
}
int c_add(int a, int b) { return a + b; }
int main() { return c_add(2, 3); }
