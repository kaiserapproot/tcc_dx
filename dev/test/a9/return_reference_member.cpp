struct M {
    int value;
    ~M() {}
};

struct R {
    M& ref;
    R(M& value) : ref(value) {}
};

R make(R& source)
{
    return source;
}

int consume(R source)
{
    return source.ref.value;
}

int main()
{
    M value;
    R source(value);

    value.value = 41;
    return consume(make(source)) == 41 ? 0 : 1;
}