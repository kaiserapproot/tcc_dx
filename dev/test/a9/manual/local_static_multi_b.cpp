extern "C" int puts(const char *);

struct B {
    ~B()
    {
        puts("DB");
    }
};

extern "C" int get_b()
{
    static B value;
    return 2;
}
