struct Base {
    ~Base() {}
};

struct Derived : Base {
};

Derived &get_derived()
{
    static Derived value;
    return value;
}

int main()
{
    get_derived();
    return 0;
}
