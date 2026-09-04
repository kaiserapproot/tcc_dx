extern "C" int puts(const char *);

struct A {
    ~A()
    {
        puts("DA");
    }
};

extern "C" int get_a()
{
    static A value;
    return 1;
}
