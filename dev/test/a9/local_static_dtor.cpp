extern "C" int printf(const char *, ...);
extern "C" int puts(const char *);

static int ctor_count;
static int copy_count;

struct P {
    int id;

    P(int value = 5)
    {
        id = value;
        ++ctor_count;
        printf("C%d", id);
        puts("");
    }

    P(const P &other)
    {
        id = other.id + 100;
        ++copy_count;
        printf("CC%d", id);
        puts("");
    }

    ~P()
    {
        printf("D%d", id);
        puts("");
    }
};

struct Q {
    int id;

    ~Q()
    {
        printf("DQ%d", id);
        puts("");
    }
};

P &get_default()
{
    static P default_obj;
    return default_obj;
}

P &get_arg(int value)
{
    static P arg_obj(value);
    return arg_obj;
}

P &get_copy()
{
    static P source_obj(11);
    static P copy_obj = source_obj;
    return copy_obj;
}

Q &get_plain()
{
    static Q plain_obj;
    plain_obj.id = 13;
    return plain_obj;
}

void never_called()
{
    static P unused_obj(99);
}

int main()
{
    P &a = get_default();
    P &b = get_default();
    P &c = get_arg(7);
    P &d = get_arg(8);
    P &e = get_copy();
    P &f = get_copy();
    Q &g = get_plain();

    if (a.id != 5 || b.id != 5)
        return 1;
    if (c.id != 7 || d.id != 7)
        return 2;
    if (e.id != 111 || f.id != 111)
        return 3;
    if (g.id != 13)
        return 4;
    if (ctor_count != 3 || copy_count != 1)
        return 5;
    puts("END");
    return 0;
}
