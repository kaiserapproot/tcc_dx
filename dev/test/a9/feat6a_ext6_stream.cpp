// FEAT-6A-ext6: the idiomatic non-member operator<< returning T& enables
// stream-style chaining `sink << a << b << c` (left-associative, each call
// returns the sink reference).
class Sink {
public:
    int total;
};

Sink& operator<<(Sink& s, int x) { s.total += x; return s; }

int main()
{
    Sink s;
    s.total = 0;
    s << 3 << 7 << 10;       // ((s << 3) << 7) << 10
    return s.total - 20;     // 3 + 7 + 10 - 20 = 0
}
