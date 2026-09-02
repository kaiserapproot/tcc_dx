struct S {
    ~S() {}
};

S *get_saved(S *source)
{
    static S *saved;
    saved = source;
    return saved;
}

int main()
{
    S value;
    return get_saved(&value) == &value ? 0 : 1;
}