// N6-02 fail-closed: constructor を持つが 0 引数で呼べる overload が無い型.
// 期待: "thread_local object of class without default constructor is unsupported"
struct P {
    int value;
    P(int a) { value = a; }
};

thread_local P object;

int main()
{
    return object.value;
}
