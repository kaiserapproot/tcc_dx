struct M {
    int value;
    ~M() {}
};

struct R {
    M& ref;
};

R make(R& source)
{
    return source;
}

int main()
{
    return 0;
}