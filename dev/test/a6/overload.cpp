int f(int x) { return x; }
double f(double x) { return x; }
int main() {
    if (f(1) != 1) return 1;
    if (f(1.0) != 1.0) return 2;
    return 0;
}