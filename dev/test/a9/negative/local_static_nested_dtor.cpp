struct M {
    ~M() {}
};

struct H {
    M member;
};

void use_static()
{
    static H value;
}

int main()
{
    use_static();
    return 0;
}
