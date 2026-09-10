// N7-00 case C: local auto, no viable default ctor — POST-N6 silent miscompile (SA-01).
class P {
public:
    P(int x) { v = x; }
    int v;
};
int main(void)
{
    P f;
    return f.v;
}
