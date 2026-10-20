class Counter {
public:
    int n;
    Counter(int start);
    ~Counter();
    int bump(int by);
};
int twice(int x);
double twice(double x);
extern Counter g_counter;
