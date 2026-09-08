// N7-00 storage LOCAL_STATIC: no viable default ctor.
struct P {
    P(int x) { (void)x; }
};
int main(void)
{
    static P static_a;
    (void)static_a;
    return 0;
}
