class Counter { public: static int count; };
int Counter::count = 0;
int main() {
    Counter::count;
    return 0;
}
