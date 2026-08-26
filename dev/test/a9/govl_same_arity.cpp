// BUG-30 / G-OVL: same-arity overloads are the silent-miscompile case -
// argument counts match, so nothing errors out; only the run-time value
// shows which body ran.  Definitions come after both call sites.
class S {
public:
    int m;
    int pick(int a);
    int pick(const char* p);
    int pick(double d);
};
int callInt(S& s) { return s.pick(1); }
int callStr(S& s) { return s.pick("x"); }
int callDbl(S& s) { return s.pick(2.5); }
int S::pick(int a) { m = 1; return 1; }
int S::pick(const char* p) { m = 2; return 2; }
int S::pick(double d) { m = 3; return 3; }
int main()
{
    S s;
    s.m = 0;
    if (callInt(s) != 1 || s.m != 1)
        return 1;
    if (callStr(s) != 2 || s.m != 2)
        return 2;
    if (callDbl(s) != 3 || s.m != 3)
        return 3;
    return 0;
}
