// Function-local static copy-initialization must construct exactly once and
// retain the value from the first source object.
int copies;

struct P {
    int value;
    P(int v) { value = v; }
    P(const P& other) { value = other.value; copies++; }
};

P& get_static(P& source)
{
    static P saved = source;
    return saved;
}

int main()
{
    P first(11);
    P second(22);
    P& a = get_static(first);
    P& b = get_static(second);

    if (&a != &b)
        return 1;
    if (copies != 1)
        return 2;
    if (a.value != 11 || b.value != 11)
        return 3;
    return 0;
}