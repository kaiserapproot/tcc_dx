class Counter { public: static int count; };
int Counter::count = 0;
int main() { Counter::count = 5; return 0; }
