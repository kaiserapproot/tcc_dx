struct S {
    int value;

    S(int n) { value = n; }
    S(const S& other) { value = other.value; }
    ~S() {}
};

void save(S& source)
{
    static S saved = source;
}

int main()
{
    return 0;
}