// Out-of-class definition of forward-declared nested classes - the
// TestRunner.cpp:31 shape (`class TestRunner::Utility { ... };`),
// including a second nested class, out-of-class member definitions with
// double qualification (G3-P4), and use of the OUTER class's typedefs
// inside the out-of-class body (G3 scope walk via the qualifier).
struct Runner {
    typedef int counter_t;
    class Utility;              /* forward declarations only */
    class Logger;
    int r;
};
class Runner::Utility {
  public:
    counter_t doubleIt(counter_t v);        /* outer typedef visible */
    static int add(int a, int b) { return a + b; }
    int u;
};
class Runner::Logger {
  public:
    Logger(int base) : b(base) {}
    int log(int x);
    int b;
};
Runner::counter_t Runner::Utility::doubleIt(counter_t v)
{
    return v * 2;
}
int Runner::Logger::log(int x)
{
    return b + x;
}
int main()
{
    Runner::Utility u;
    Runner::Logger lg(10);

    u.u = 1;
    if (u.doubleIt(21) != 42)
        return 1;
    if (Runner::Utility::add(2, 3) != 5)
        return 2;
    if (lg.log(5) != 15)
        return 3;
    return 0;
}
