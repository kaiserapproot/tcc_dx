class Counter { public: static int count; };
int Counter::count = 0;
int main() { return Counter::count; }