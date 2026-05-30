class Counter { public: static int count; };
int Counter::count = 0;
void f() { Counter::count = 5; }
int main() { return 0; }
