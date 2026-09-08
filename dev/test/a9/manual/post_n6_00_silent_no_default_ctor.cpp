// POST-N6-00: silent acceptance probe — local auto without viable default ctor.
// Expected inventory result: COMPILE=PASS, RUNTIME=UB, CLASS=SILENT_ACCEPTANCE.
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
