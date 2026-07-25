// FEAT-6A-ext4: integration - member prefix/postfix ++ and -- together.
class Counter {
public:
    int v;
    Counter& operator++() { v = v + 1; return *this; }
    Counter operator++(int) { Counter old; old.v = v; v = v + 1; return old; }
    Counter& operator--() { v = v - 1; return *this; }
    Counter operator--(int) { Counter old; old.v = v; v = v - 1; return old; }
};

int main()
{
    Counter c;
    c.v = 10;
    Counter a = c++;   // a.v = 10, c.v = 11
    Counter b = c--;   // b.v = 11, c.v = 10
    ++c;               // c.v = 11
    --c;               // c.v = 10
    return a.v + b.v + c.v - 31;   // 10 + 11 + 10 - 31 = 0
}
