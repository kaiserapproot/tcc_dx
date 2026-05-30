class Counter { public: static int count; static int get(); };
int Counter::count = 0;
int Counter::get() { return count; }
int main() {
    int x = Counter::count;
    return Counter::get() == 0 ? 0 : 1;
}
