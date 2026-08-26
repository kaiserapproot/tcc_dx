// G3 P5: a call-site local named like the default argument must NOT be
// captured by the replayed tokens - `= npos` means the DEFINING scope's
// S::npos, never the caller's local (this was a silent-miscompile risk,
// verified failing before the fix).
class S {
public:
    static const int npos;
    int last;
    void f(int n = npos);
};
const int S::npos = 41;
void S::f(int n)
{
    last = n;
}
int main()
{
    S s;
    int npos = 999;
    s.f();
    if (s.last != 41)
        return 1;
    return npos == 999 ? 0 : 2;
}
