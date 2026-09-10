// N7-00 storage GLOBAL: no viable default ctor at namespace scope.
struct P {
    P(int x) { (void)x; }
};
P global_a;
int main(void)
{
    return 0;
}
