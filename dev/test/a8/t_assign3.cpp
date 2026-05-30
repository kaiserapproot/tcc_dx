class Counter { public: static int count; };
int Counter::count = 0;
void f() { int x; x = Counter::count; }
int main() { return 0; }
