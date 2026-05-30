class Counter { public: static int count; static int get(); };
int Counter::count = 0;
int Counter::get() { return count; }
int main() {
    return Counter::count = 5, Counter::get() - 5;
}
